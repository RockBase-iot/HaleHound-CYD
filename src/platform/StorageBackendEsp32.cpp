#include "platform/StorageBackend.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "cyd_config.h"

namespace {
class ArduinoStorageBackend final : public IStorageBackend {
public:
    StorageCode begin() override {
        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);
        SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
        return SD.begin(SD_CS, SPI, 4000000) ? StorageCode::Ok : StorageCode::Unavailable;
    }
    StorageCode write(const char* path, const uint8_t* data, size_t size) override {
        SD.remove(path);
        File file = SD.open(path, FILE_WRITE);
        if (!file) return StorageCode::Fault;
        const size_t written = file.write(data, size);
        file.close();
        return written == size ? StorageCode::Ok : StorageCode::Fault;
    }
    StorageCode flush(const char*) override { return StorageCode::Ok; }
    bool verify(const char* path, size_t size, uint32_t expected) override {
        File file = SD.open(path, FILE_READ);
        if (!file || file.size() != size) {
            if (file) file.close();
            return false;
        }
        uint32_t actual = 2166136261UL;
        while (file.available()) {
            const int value = file.read();
            if (value < 0) break;
            actual ^= static_cast<uint8_t>(value);
            actual *= 16777619UL;
        }
        file.close();
        return actual == expected;
    }
    StorageCode rename(const char* from, const char* to) override {
        return SD.rename(from, to) ? StorageCode::Ok : StorageCode::Fault;
    }
    StorageCode ensureDirectory(const char* path) override {
        return SD.exists(path) || SD.mkdir(path) ? StorageCode::Ok : StorageCode::Fault;
    }
    StorageResult migrateLegacyCredentials() override {
        if (!SD.exists("/creds.txt")) {
            return {StorageCode::NotFound, 0U, "legacy credentials not found"};
        }
        if (SD.exists("/loot/credentials/legacy_creds.txt")) {
            return {StorageCode::Ok, 0U, "legacy credentials already migrated"};
        }
        if (!SD.exists("/loot")) SD.mkdir("/loot");
        if (!SD.exists("/loot/credentials")) SD.mkdir("/loot/credentials");
        File source = SD.open("/creds.txt", FILE_READ);
        File destination = SD.open("/loot/credentials/legacy_creds.txt", FILE_WRITE);
        if (!source || !destination) {
            if (source) source.close();
            if (destination) destination.close();
            return {StorageCode::Fault, 0U, "legacy credentials migration open failed"};
        }
        size_t copied = 0U;
        char line[192]{};
        size_t length = 0U;
        while (source.available()) {
            const int value = source.read();
            if (value < 0) break;
            if (value == '\n' || length + 1U >= sizeof(line)) {
                line[length] = '\0';
                if (isLegacyPskRecord(line)) copied += destination.println(line);
                length = 0U;
            } else if (value != '\r') {
                line[length++] = static_cast<char>(value);
            }
        }
        if (length > 0U) {
            line[length] = '\0';
            if (isLegacyPskRecord(line)) copied += destination.println(line);
        }
        source.close();
        destination.close();
        return {StorageCode::Ok, copied, "legacy credentials migrated"};
    }
};

ArduinoStorageBackend backend;
}

IStorageBackend& platformStorageBackend() { return backend; }
#else
namespace {
class UnavailableStorageBackend final : public IStorageBackend {
public:
    StorageCode begin() override { return StorageCode::Unavailable; }
    StorageCode write(const char*, const uint8_t*, size_t) override { return StorageCode::Unavailable; }
    StorageCode flush(const char*) override { return StorageCode::Unavailable; }
    bool verify(const char*, size_t, uint32_t) override { return false; }
    StorageCode rename(const char*, const char*) override { return StorageCode::Unavailable; }
};

UnavailableStorageBackend backend;
}

IStorageBackend& platformStorageBackend() { return backend; }
#endif
