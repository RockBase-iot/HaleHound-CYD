#pragma once

// Unified NDEF codec.
//
// Upstream v4.0 consolidated four separate NDEF implementations (a decoder,
// eight record builders and a URI prefix table, duplicated across the PN532,
// NTAG Tools, Chameleon and ST25R3916 paths) into a single file that is
// host-tested without an ESP32 toolchain. This is that single file.
//
// Scope and conventions
// ---------------------
// * The message/record framing is NFC Forum NDEF 1.0 (header byte, type
//   length, payload length, optional ID, type, payload).
// * Record payloads follow the NFC Forum well-known types where one exists
//   (U, T, text/vcard, geo) and the Wi-Fi Simple Configuration TLV layout for
//   Wi-Fi credentials.
// * Two records have no NFC Forum well-known type and therefore use the
//   de-facto convention rather than a standard: Android app records carry the
//   external type ``android.com:pkg``, and SMS records are emitted as a URI
//   record with the ``sms:`` scheme. Both choices are explicit here so a
//   reviewer can change them in one place.
// * Nothing in this file performs I/O. It builds and parses byte buffers; the
//   tag drivers own the transport.
//
// Capacity notes
// --------------
// NDEF on a tag lives inside a Type 2 TLV. ``encodeTlv`` wraps a message for
// writing; ``decodeTlv`` unwraps one read back off a tag, tolerating the
// no-message terminator and the reserved TLV used for padding.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class IFeature;

enum class NdefRecordType : uint8_t {
    Url,
    Text,
    VCard,
    Wifi,
    Bluetooth,
    AndroidApp,
    Geo,
    Sms,
    Unknown,
};

struct NdefRecord {
    NdefRecordType type = NdefRecordType::Unknown;
    std::string typeName;   // NDEF TYPE field, verbatim
    std::vector<uint8_t> payload;
};

struct NdefMessage {
    std::vector<NdefRecord> records;
};

struct NdefVCard {
    std::string formattedName;
    std::string organization;
    std::string phone;
    std::string email;
    std::string url;
};

struct NdefWifiCredential {
    std::string ssid;
    std::string password;
    // 0x0001 open, 0x0002 WPA-PSK, 0x0020 WPA2-PSK (Wi-Fi Simple Config).
    uint16_t authType = 0x0020;
    // 0x0001 none, 0x0008 AES.
    uint16_t encryptionType = 0x0008;
};

struct NdefBluetoothOob {
    // Colon-separated or bare hex; stored inverted per Bluetooth OOB spec.
    std::string deviceAddress;
    std::string deviceName;
};

struct NdefEncodeResult {
    bool valid = false;
    const char* detail = "encode failed";
    std::vector<uint8_t> bytes;
};

struct NdefDecodeResult {
    bool valid = false;
    const char* detail = "decode failed";
    NdefMessage message;
};

class NdefCodec {
public:
    // ---- URI prefix table -------------------------------------------------
    // NFC Forum URI RTD abbreviations, code 0x00-0x23.
    static const char* uriPrefix(uint8_t code);
    static uint8_t uriPrefixCount();
    // Returns the shortest matching abbreviation code for a URL and fills
    // ``rest`` with the remainder. Falls back to code 0x00 (no abbreviation).
    static uint8_t abbreviateUri(const std::string& uri, std::string& rest);
    static std::string expandUri(uint8_t code, const std::string& rest);

    // ---- builders ---------------------------------------------------------
    static NdefRecord buildUrl(const std::string& url);
    static NdefRecord buildText(const std::string& text, const std::string& language = "en");
    static NdefRecord buildVCard(const NdefVCard& card);
    static NdefRecord buildWifi(const NdefWifiCredential& credential);
    static NdefRecord buildBluetooth(const NdefBluetoothOob& oob);
    static NdefRecord buildAndroidApp(const std::string& packageName);
    static NdefRecord buildGeo(double latitude, double longitude);
    static NdefRecord buildSms(const std::string& number, const std::string& body);

    // ---- message encode / decode -----------------------------------------
    static NdefEncodeResult encode(const NdefMessage& message);
    static NdefDecodeResult decode(const std::vector<uint8_t>& bytes);

    // ---- Type 2 TLV wrapper ----------------------------------------------
    static NdefEncodeResult encodeTlv(const NdefMessage& message);
    static NdefDecodeResult decodeTlv(const std::vector<uint8_t>& bytes);

    // ---- presentation -----------------------------------------------------
    // Human-readable one-line summary used by the on-device record viewer and
    // the Loot text export. Never fails.
    static std::string summarize(const NdefRecord& record);
    static const char* typeLabel(NdefRecordType type);
};

// Appends ``record`` to the message. Staged records stack instead of replacing
// each other, which is the behaviour the NTAG Tools composer relies on.
void ndefAppend(NdefMessage& message, const NdefRecord& record);
// Removes every staged record without touching the tag.
void ndefClear(NdefMessage& message);

IFeature* createNtagToolsFeature();
