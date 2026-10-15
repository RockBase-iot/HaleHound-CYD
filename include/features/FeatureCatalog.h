#pragma once

#include <array>
#include <cstddef>

#include "core/FeatureTypes.h"
#include "drivers/PeripheralIds.h"

class IFeature;

enum class CategoryId : uint8_t {
    Wifi,
    Bluetooth,
    Nrf24,
    SubGhz,
    Rfid,
    Sigint,
    Defensive,
    Tools,
    Settings,
    Ir,
    Rf433,
};

enum class FeatureId : uint16_t {
    WifiScanner = 0x0100,
    WifiDeauth = 0x0101,
    WifiPacketMonitor = 0x0102,
    WifiBeaconSpammer = 0x0103,
    WifiProbeSniffer = 0x0104,
    WifiCaptivePortal = 0x0105,
    WifiStationScanner = 0x0106,
    WifiAuthFlood = 0x0107,
    BluetoothScanner = 0x0200,
    BluetoothJammer = 0x0201,
    BluetoothSpoofer = 0x0202,
    BluetoothBeacon = 0x0203,
    BluetoothSniffer = 0x0204,
    BluetoothWhisperPair = 0x0205,
    BluetoothAirTag = 0x0206,
    BluetoothLunaticFringe = 0x0207,
    BluetoothDucky = 0x0208,
    BluetoothPredator = 0x0209,
    BluetoothAirohaRace = 0x020A,
    BluetoothSkeletonKey = 0x020B,
    BluetoothHidReader = 0x020C,
    BluetoothKarr = 0x020D,
    Nrf24Scanner = 0x0300,
    Nrf24Spectrum = 0x0301,
    Nrf24Sniffer = 0x0302,
    Nrf24MouseJack = 0x0303,
    Nrf24WlanJammer = 0x0304,
    Nrf24ProtoKill = 0x0305,
    Nrf24Tuning = 0x0306,
    SubGhzReplay = 0x0400,
    SubGhzBruteForce = 0x0401,
    SubGhzJammer = 0x0402,
    SubGhzSpectrum = 0x0403,
    SubGhzSavedProfiles = 0x0404,
    SubGhzRead = 0x0405,
    TeslaCharge = 0x0406,
    SubGhzTuning = 0x0407,
    RfidScanner = 0x0500,
    RfidReader = 0x0501,
    RfidClone = 0x0502,
    RfidKeyBruteForce = 0x0503,
    RfidEmulate = 0x0504,
    NtagTools = 0x0505,
    RfidBackend = 0x0506,
    JamDetect = 0x0600,
    WifiGuardian = 0x0601,
    SubGhzSentinel = 0x0602,
    Nrf24Watchdog = 0x0603,
    SigintKarma = 0x0700,
    SigintWardriving = 0x0701,
    SigintSavedCaptures = 0x0702,
    SigintIotRecon = 0x0703,
    SigintLoot = 0x0704,
    SigintEapolCapture = 0x0705,
    SigintFlockYou = 0x0706,
    DroneDetect = 0x0707,
    DroneRidSpoof = 0x0708,
    DroneJelly = 0x0709,
    DroneDeauth = 0x070A,
    SkimmerHunt = 0x070B,
    ToolSerialMonitor = 0x0800,
    ToolFirmwareUpdate = 0x0801,
    ToolTouchCalibrate = 0x0802,
    ToolGps = 0x0803,
    ToolRadioTest = 0x0804,
    SettingBrightness = 0x0900,
    SettingScreenTimeout = 0x0901,
    SettingColorSwap = 0x0902,
    SettingInvertDisplay = 0x0903,
    SettingColorMode = 0x0904,
    SettingRotation = 0x0905,
    SettingDeviceInfo = 0x0906,
    SettingPin = 0x0907,
    SettingCc1101Module = 0x0908,
    IrTools = 0x0B00,
    Rf433Tools = 0x0A00,
};

using FeatureFactory = IFeature* (*)();

struct FeatureDescriptor {
    FeatureId id;
    CategoryId category;
    const char* displayName;
    RiskClass risk;
    std::array<PeripheralId, 4> requiredPeripherals;
    uint8_t requiredPeripheralCount;
    std::array<PeripheralId, 4> optionalPeripherals;
    uint8_t optionalPeripheralCount;
    const char* service;
    const char* storagePath;
    FeatureFactory factory;
};

class FeatureRegistry;
const FeatureRegistry& featureCatalog();
