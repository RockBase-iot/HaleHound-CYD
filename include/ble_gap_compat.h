#ifndef BLE_GAP_COMPAT_H
#define BLE_GAP_COMPAT_H

#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <esp_err.h>

typedef uint8_t esp_ble_addr_type_t;

struct esp_ble_adv_params_t {
    uint16_t adv_int_min;
    uint16_t adv_int_max;
    uint8_t adv_type;
    uint8_t own_addr_type;
    uint8_t channel_map;
    uint8_t adv_filter_policy;
};

#define ADV_TYPE_NONCONN_IND BLE_GAP_CONN_MODE_NON
#define BLE_ADDR_TYPE_RANDOM BLE_OWN_ADDR_RANDOM
#define ADV_CHNL_ALL 0x07
#define ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY 0x00

inline esp_err_t esp_ble_gap_stop_advertising() {
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    return advertising == nullptr || advertising->stop() ? ESP_OK : ESP_FAIL;
}

inline esp_err_t esp_ble_gap_set_rand_addr(uint8_t* address) {
    if (!BLEDevice::setOwnAddr(address)) {
        return ESP_FAIL;
    }
    return BLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM) ? ESP_OK : ESP_FAIL;
}

inline esp_err_t esp_ble_gap_config_adv_data_raw(uint8_t* payload, uint32_t length) {
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    if (advertising == nullptr) {
        return ESP_FAIL;
    }

    BLEAdvertisementData data;
    data.addData(reinterpret_cast<char*>(payload), length);
    return advertising->setAdvertisementData(data) ? ESP_OK : ESP_FAIL;
}

inline esp_err_t esp_ble_gap_start_advertising(esp_ble_adv_params_t* params) {
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    if (advertising == nullptr || params == nullptr) {
        return ESP_FAIL;
    }

    advertising->setMinInterval(params->adv_int_min);
    advertising->setMaxInterval(params->adv_int_max);
    advertising->setAdvertisementType(params->adv_type);
    advertising->setScanFilter(false, false);
    return advertising->start() ? ESP_OK : ESP_FAIL;
}

inline esp_err_t esp_ble_tx_power_set(esp_ble_power_type_t powerType, esp_power_level_t powerLevel) {
    BLEDevice::setPower(powerLevel, powerType);
    return ESP_OK;
}

#endif