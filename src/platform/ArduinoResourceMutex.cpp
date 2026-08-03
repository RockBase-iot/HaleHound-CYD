#include "platform/ArduinoResourceMutex.h"

#ifdef ARDUINO

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

ArduinoResourceMutex::ArduinoResourceMutex()
    : handle_(xSemaphoreCreateMutex()) {}

ArduinoResourceMutex::~ArduinoResourceMutex() {
    if (handle_ != nullptr) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(handle_));
    }
}

bool ArduinoResourceMutex::lock(uint32_t timeoutMs) {
    if (handle_ == nullptr) {
        return false;
    }
    const TickType_t ticks = timeoutMs == 0 ? 0 : pdMS_TO_TICKS(timeoutMs);
    return xSemaphoreTake(static_cast<SemaphoreHandle_t>(handle_), ticks) == pdTRUE;
}

void ArduinoResourceMutex::unlock() {
    if (handle_ != nullptr) {
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(handle_));
    }
}

#else

ArduinoResourceMutex::ArduinoResourceMutex() = default;
ArduinoResourceMutex::~ArduinoResourceMutex() = default;

bool ArduinoResourceMutex::lock(uint32_t) {
    if (locked_) {
        return false;
    }
    locked_ = true;
    return true;
}

void ArduinoResourceMutex::unlock() {
    locked_ = false;
}

#endif
