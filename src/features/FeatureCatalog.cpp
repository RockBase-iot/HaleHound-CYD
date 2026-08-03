#include "features/FeatureCatalog.h"

#include "core/FeatureRegistry.h"
#include "core/IFeature.h"
#include "core/FeatureRunner.h"
#include "features/subghz/SubFileParser.h"
#include "features/rfid/NtagModel.h"
#include "features/LegacyFeatureFactories.h"
#include "features/ble/BlePredatorModel.h"
#include "features/sigint/FlockFingerprint.h"
#include "features/drone/RemoteIdParser.h"
#include "features/skimmer/SkimmerScore.h"
#include "features/ir/IrFeature.h"
#include "features/rf433/Rf433Feature.h"

namespace {
constexpr std::array<PeripheralId, 4> none{};
constexpr std::array<PeripheralId, 4> diagnosticOptional{
    PeripheralId::Cc1101, PeripheralId::Nrf24, PeripheralId::Pn532, PeripheralId::Gps};
constexpr std::array<PeripheralId, 4> irOptional{
    PeripheralId::IrTx, PeripheralId::IrRx, PeripheralId::IrTx, PeripheralId::IrRx};
constexpr std::array<PeripheralId, 4> rf433Optional{
    PeripheralId::Rf433Tx, PeripheralId::Rf433Rx, PeripheralId::Rf433Tx, PeripheralId::Rf433Rx};

class CatalogFeatureStub final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        running_ = true;
        return {FeatureResultCode::Completed, "feature ready"};
    }

    void tick() override {}

    void requestStop(StopReason) override { running_ = false; }

    FeatureResult end() override {
        running_ = false;
        return {FeatureResultCode::Completed, "feature ended"};
    }

private:
    bool running_ = false;
};

IFeature* makeCatalogFeature() {
    return new CatalogFeatureStub();
}

#define F makeCatalogFeature
#define D(id, category, name, risk, required, requiredCount, optional, optionalCount, service, path) \
    {FeatureId::id, CategoryId::category, name, RiskClass::risk, required, requiredCount, optional, optionalCount, service, path, F}
#define D_FACTORY(id, category, name, risk, required, requiredCount, optional, optionalCount, service, path, factory) \
    {FeatureId::id, CategoryId::category, name, RiskClass::risk, required, requiredCount, optional, optionalCount, service, path, factory}
#define DL(id, category, name, risk, required, requiredCount, optional, optionalCount, service, path, factory) \
    {FeatureId::id, CategoryId::category, name, RiskClass::risk, required, requiredCount, optional, optionalCount, service, path, factory}

const FeatureDescriptor kDescriptors[] = {
    DL(WifiPacketMonitor, Wifi, "Packet Monitor", Passive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiPacketMonitor),
    DL(WifiBeaconSpammer, Wifi, "Beacon Spammer", Offensive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiBeaconSpammer),
    DL(WifiDeauth, Wifi, "WiFi Deauther", Offensive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiDeauther),
    DL(WifiProbeSniffer, Wifi, "Probe Sniffer", Passive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiProbeSniffer),
    DL(WifiScanner, Wifi, "WiFi Scanner", Passive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiScanner),
    DL(WifiCaptivePortal, Wifi, "Captive Portal", Offensive, none, 0, none, 0, "wifi", "/loot", createLegacyWifiCaptivePortal),
    DL(WifiStationScanner, Wifi, "Station Scanner", Passive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiStationScanner),
    DL(WifiAuthFlood, Wifi, "Auth Flood", Offensive, none, 0, none, 0, "wifi", "/captures", createLegacyWifiAuthFlood),

    DL(BluetoothJammer, Bluetooth, "BLE Jammer", Offensive, none, 0, none, 0, "ble", "/captures", createLegacyBluetoothJammer),
    DL(BluetoothSpoofer, Bluetooth, "BLE Spoofer", Offensive, none, 0, none, 0, "ble", "/loot", createLegacyBluetoothSpoofer),
    DL(BluetoothBeacon, Bluetooth, "BLE Beacon", Passive, none, 0, none, 0, "ble", "/captures", createLegacyBluetoothBeacon),
    DL(BluetoothSniffer, Bluetooth, "BLE Sniffer", Passive, none, 0, none, 0, "ble", "/captures", createLegacyBluetoothSniffer),
    DL(BluetoothScanner, Bluetooth, "BLE Scanner", Passive, none, 0, none, 0, "ble", "/captures", createLegacyBluetoothScanner),
    DL(BluetoothWhisperPair, Bluetooth, "WhisperPair", Offensive, none, 0, none, 0, "ble", "/loot", createLegacyBluetoothWhisperPair),
    DL(BluetoothAirTag, Bluetooth, "AirTag", Offensive, none, 0, none, 0, "ble", "/loot", createLegacyBluetoothAirTag),
    DL(BluetoothLunaticFringe, Bluetooth, "Lunatic Fringe", Offensive, none, 0, none, 0, "ble", "/loot", createLegacyBluetoothLunaticFringe),
    DL(BluetoothDucky, Bluetooth, "BLE Ducky", Offensive, none, 0, none, 0, "ble", "/loot", createLegacyBluetoothDucky),
    D_FACTORY(BluetoothPredator, Bluetooth, "BLE Predator", SensitiveData, none, 0, none, 0, "ble", "/wp_loot", createBlePredatorFeature),
    D_FACTORY(BluetoothAirohaRace, Bluetooth, "Airoha Race", Offensive, none, 0, none, 0, "ble", "/loot", createAirohaRaceFeature),

    DL(Nrf24Scanner, Nrf24, "nRF24 Scanner", Passive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24Scanner),
    DL(Nrf24Spectrum, Nrf24, "nRF24 Spectrum Analyzer", Passive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24Spectrum),
    DL(Nrf24Sniffer, Nrf24, "nRF24 Sniffer", Passive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24Sniffer),
    DL(Nrf24MouseJack, Nrf24, "MouseJack", Offensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/loot", createLegacyNrf24MouseJack),
    DL(Nrf24WlanJammer, Nrf24, "WLAN Jammer", Offensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24WlanJammer),
    DL(Nrf24ProtoKill, Nrf24, "Proto Kill", Offensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24ProtoKill),

    DL(SubGhzReplay, SubGhz, "SubGHz Replay", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createLegacySubGhzReplay),
    DL(SubGhzBruteForce, SubGhz, "SubGHz Brute Force", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createLegacySubGhzBruteForce),
    DL(SubGhzJammer, SubGhz, "SubGHz Jammer", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createLegacySubGhzJammer),
    DL(SubGhzSpectrum, SubGhz, "SubGHz Spectrum Analyzer", Passive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/captures", createLegacySubGhzSpectrum),
    D(SubGhzSavedProfiles, SubGhz, "Saved Profiles", Passive, none, 0, {PeripheralId::Cc1101}, 1, "storage", "/subghz"),
    D_FACTORY(SubGhzRead, SubGhz, "SubGHz File Reader", Passive, none, 0, {PeripheralId::Cc1101}, 1, "storage", "/subghz", createSubReadFeature),
    D_FACTORY(TeslaCharge, SubGhz, "Tesla Charge", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createTeslaChargeFeature),

    DL(RfidScanner, Rfid, "Card Scanner", SensitiveData, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createLegacyRfidScanner),
    DL(RfidReader, Rfid, "Card Reader", SensitiveData, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createLegacyRfidReader),
    DL(RfidClone, Rfid, "Card Clone", Offensive, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createLegacyRfidClone),
    DL(RfidKeyBruteForce, Rfid, "Key Brute Force", Offensive, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createLegacyRfidKeyBruteForce),
    DL(RfidEmulate, Rfid, "Card Emulate", Offensive, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createLegacyRfidEmulate),
    D_FACTORY(NtagTools, Rfid, "NTAG Tools", SensitiveData, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createNtagToolsFeature),

    DL(JamDetect, Defensive, "Jam Detect", Defensive, none, 0, none, 0, "diagnostics", "/logs", createLegacyFullSpectrum),
    DL(WifiGuardian, Defensive, "WiFi Guardian", Defensive, none, 0, none, 0, "diagnostics", "/logs", createLegacyWifiGuardian),
    DL(SubGhzSentinel, Defensive, "SubGHz Sentinel", Defensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/logs", createLegacySubGhzSentinel),
    DL(Nrf24Watchdog, Defensive, "2.4GHz Watchdog", Defensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/logs", createLegacyNrf24Watchdog),

    DL(SigintKarma, Sigint, "Karma Attack", Offensive, none, 0, none, 0, "wifi", "/loot", createLegacySigintKarma),
    D(SigintWardriving, Sigint, "Wardriving", Passive, {PeripheralId::Gps}, 1, {PeripheralId::Sd}, 1, "gps", "/wardriving"),
    DL(SigintSavedCaptures, Sigint, "Saved Captures", Passive, none, 0, {PeripheralId::Sd}, 1, "storage", "/captures", createLegacySigintSavedCaptures),
    DL(SigintIotRecon, Sigint, "IoT Recon", Passive, none, 0, {PeripheralId::Sd}, 1, "storage", "/loot", createLegacySigintIotRecon),
    DL(SigintLoot, Sigint, "Loot Viewer", SensitiveData, none, 0, {PeripheralId::Sd}, 1, "storage", "/loot", createLegacySigintLoot),
    DL(SigintEapolCapture, Sigint, "EAPOL Capture", SensitiveData, none, 0, {PeripheralId::Sd}, 1, "wifi", "/eapol", createLegacySigintEapolCapture),
    D_FACTORY(SigintFlockYou, Sigint, "Flock You", SensitiveData, none, 0, {PeripheralId::Gps}, 1, "ble", "/loot/flock", createFlockYouFeature),
    D_FACTORY(DroneDetect, Sigint, "Drone Detect", Passive, none, 0, none, 0, "remote-id", "/captures", createDroneDetectFeature),
    D_FACTORY(DroneRidSpoof, Sigint, "RID Spoof", Offensive, none, 0, none, 0, "remote-id", "/loot", createRidSpoofFeature),
    D_FACTORY(DroneJelly, Sigint, "Drone Jelly", Offensive, none, 0, none, 0, "wifi", "/loot", createDroneJellyFeature),
    D_FACTORY(DroneDeauth, Sigint, "Drone Deauth", Offensive, none, 0, none, 0, "wifi", "/loot", createDroneDeauthFeature),
    D_FACTORY(SkimmerHunt, Sigint, "Skimmer Hunt", SensitiveData, none, 0, none, 0, "ble", "/loot/skimmer", createSkimmerHuntFeature),

    D(ToolSerialMonitor, Tools, "Serial Monitor", Passive, none, 0, none, 0, "uart", "/logs"),
    D(ToolFirmwareUpdate, Tools, "Update Firmware", Passive, {PeripheralId::Sd}, 1, none, 0, "storage", "/firmware"),
    D(ToolTouchCalibrate, Tools, "Touch Calibrate", Passive, none, 0, none, 0, "display", "/settings"),
    D(ToolGps, Tools, "GPS", Passive, {PeripheralId::Gps}, 1, none, 0, "gps", "/wardriving"),
    D(ToolRadioTest, Tools, "Radio Test", Defensive, none, 0, diagnosticOptional, 3, "diagnostics", "/logs"),

    D(SettingBrightness, Settings, "Brightness", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingScreenTimeout, Settings, "Screen Timeout", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingColorSwap, Settings, "Swap Colors", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingInvertDisplay, Settings, "Invert Display", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingColorMode, Settings, "Color Mode", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingRotation, Settings, "Rotation", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingDeviceInfo, Settings, "Device Info", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingPin, Settings, "Set PIN", Passive, none, 0, none, 0, "settings", "/settings"),
    D(SettingCc1101Module, Settings, "CC1101 Module", Passive, none, 0, none, 0, "settings", "/settings"),
    D_FACTORY(IrTools, Ir, "Infrared Tools", Passive, none, 0, irOptional, 2, "gpio", "/loot/ir", createIrFeature),
    D_FACTORY(Rf433Tools, Rf433, "RF433 Tools", Passive, none, 0, rf433Optional, 2, "gpio", "/loot/rf433", createRf433Feature),
};

#undef D
#undef D_FACTORY
#undef DL
#undef F

const FeatureRegistry kRegistry(kDescriptors, sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}

const FeatureRegistry& featureCatalog() {
    return kRegistry;
}
