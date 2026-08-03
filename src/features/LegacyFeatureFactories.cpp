#include "features/LegacyFeatureFactories.h"

#include "features/LegacyFeatureAdapter.h"

#ifdef ARDUINO
#include "wifi_attacks.h"
#include "bluetooth_attacks.h"
#include "nrf24_attacks.h"
#include "subghz_attacks.h"
#include "rfid_attacks.h"
#include "jam_detect.h"
#include "karma_attack.h"
#include "eapol_capture.h"
#include "saved_captures.h"
#include "iot_recon.h"
#include "loot_manager.h"
#endif

namespace {
#ifndef ARDUINO
IFeature* makeNullLegacyFeature() {
    return new LegacyFeatureAdapter({nullptr, nullptr, nullptr, nullptr});
}
#endif
}

#ifdef ARDUINO
#define LEGACY_FACTORY(factory, setupFn, loopFn, exitFn, cleanupFn) \
    IFeature* factory() { return new LegacyFeatureAdapter({setupFn, loopFn, exitFn, cleanupFn}); }
#else
#define LEGACY_FACTORY(factory, setupFn, loopFn, exitFn, cleanupFn) \
    IFeature* factory() { return makeNullLegacyFeature(); }
#endif

LEGACY_FACTORY(createLegacyWifiPacketMonitor, PacketMonitor::setup, PacketMonitor::loop, PacketMonitor::isExitRequested, PacketMonitor::cleanup)
LEGACY_FACTORY(createLegacyWifiBeaconSpammer, BeaconSpammer::setup, BeaconSpammer::loop, BeaconSpammer::isExitRequested, BeaconSpammer::cleanup)
LEGACY_FACTORY(createLegacyWifiDeauther, Deauther::setup, Deauther::loop, Deauther::isExitRequested, Deauther::cleanup)
LEGACY_FACTORY(createLegacyWifiProbeSniffer, DeauthDetect::setup, DeauthDetect::loop, DeauthDetect::isExitRequested, DeauthDetect::cleanup)
LEGACY_FACTORY(createLegacyWifiScanner, WifiScan::setup, WifiScan::loop, WifiScan::isExitRequested, WifiScan::cleanup)
LEGACY_FACTORY(createLegacyWifiCaptivePortal, CaptivePortal::setup, CaptivePortal::loop, CaptivePortal::isExitRequested, CaptivePortal::cleanup)
LEGACY_FACTORY(createLegacyWifiStationScanner, StationScan::setup, StationScan::loop, StationScan::isExitRequested, StationScan::cleanup)
LEGACY_FACTORY(createLegacyWifiAuthFlood, AuthFlood::setup, AuthFlood::loop, AuthFlood::isExitRequested, AuthFlood::cleanup)
LEGACY_FACTORY(createLegacyBluetoothJammer, BleJammer::setup, BleJammer::loop, BleJammer::isExitRequested, BleJammer::cleanup)
LEGACY_FACTORY(createLegacyBluetoothSpoofer, BleSpoofer::setup, BleSpoofer::loop, BleSpoofer::isExitRequested, BleSpoofer::cleanup)
LEGACY_FACTORY(createLegacyBluetoothBeacon, BleBeacon::setup, BleBeacon::loop, BleBeacon::isExitRequested, BleBeacon::cleanup)
LEGACY_FACTORY(createLegacyBluetoothSniffer, BleSniffer::setup, BleSniffer::loop, BleSniffer::isExitRequested, BleSniffer::cleanup)
LEGACY_FACTORY(createLegacyBluetoothScanner, BleScan::setup, BleScan::loop, BleScan::isExitRequested, BleScan::cleanup)
LEGACY_FACTORY(createLegacyBluetoothWhisperPair, WhisperPair::setup, WhisperPair::loop, WhisperPair::isExitRequested, WhisperPair::cleanup)
LEGACY_FACTORY(createLegacyBluetoothAirTag, AirTagDetect::setup, AirTagDetect::loop, AirTagDetect::isExitRequested, AirTagDetect::cleanup)
LEGACY_FACTORY(createLegacyBluetoothLunaticFringe, LunaticFringe::setup, LunaticFringe::loop, LunaticFringe::isExitRequested, LunaticFringe::cleanup)
LEGACY_FACTORY(createLegacyBluetoothDucky, BleDucky::setup, BleDucky::loop, BleDucky::isExitRequested, BleDucky::cleanup)
LEGACY_FACTORY(createLegacyNrf24Scanner, Scanner::scannerSetup, Scanner::scannerLoop, Scanner::isExitRequested, Scanner::cleanup)
LEGACY_FACTORY(createLegacyNrf24Spectrum, Analyzer::analyzerSetup, Analyzer::analyzerLoop, Analyzer::isExitRequested, Analyzer::cleanup)
LEGACY_FACTORY(createLegacyNrf24Sniffer, NrfSniffer::setup, NrfSniffer::loop, NrfSniffer::isExitRequested, NrfSniffer::cleanup)
LEGACY_FACTORY(createLegacyNrf24MouseJack, MouseJack::setup, MouseJack::loop, MouseJack::isExitRequested, MouseJack::cleanup)
LEGACY_FACTORY(createLegacyNrf24WlanJammer, WLANJammer::wlanjammerSetup, WLANJammer::wlanjammerLoop, WLANJammer::isExitRequested, WLANJammer::cleanup)
LEGACY_FACTORY(createLegacyNrf24ProtoKill, ProtoKill::prokillSetup, ProtoKill::prokillLoop, ProtoKill::isExitRequested, ProtoKill::cleanup)
LEGACY_FACTORY(createLegacySubGhzReplay, ReplayAttack::setup, ReplayAttack::loop, ReplayAttack::isExitRequested, ReplayAttack::cleanup)
LEGACY_FACTORY(createLegacySubGhzBruteForce, SubBrute::setup, SubBrute::loop, SubBrute::isExitRequested, SubBrute::cleanup)
LEGACY_FACTORY(createLegacySubGhzJammer, SubJammer::setup, SubJammer::loop, SubJammer::isExitRequested, SubJammer::cleanup)
LEGACY_FACTORY(createLegacySubGhzSpectrum, SubAnalyzer::setup, SubAnalyzer::loop, SubAnalyzer::isExitRequested, SubAnalyzer::cleanup)
LEGACY_FACTORY(createLegacyRfidScanner, RFIDScanner::setup, RFIDScanner::loop, RFIDScanner::isExitRequested, RFIDScanner::cleanup)
LEGACY_FACTORY(createLegacyRfidReader, RFIDReader::setup, RFIDReader::loop, RFIDReader::isExitRequested, RFIDReader::cleanup)
LEGACY_FACTORY(createLegacyRfidClone, RFIDClone::setup, RFIDClone::loop, RFIDClone::isExitRequested, RFIDClone::cleanup)
LEGACY_FACTORY(createLegacyRfidKeyBruteForce, RFIDBrute::setup, RFIDBrute::loop, RFIDBrute::isExitRequested, RFIDBrute::cleanup)
LEGACY_FACTORY(createLegacyRfidEmulate, RFIDEmulate::setup, RFIDEmulate::loop, RFIDEmulate::isExitRequested, RFIDEmulate::cleanup)
LEGACY_FACTORY(createLegacyWifiGuardian, WiFiGuardian::setup, WiFiGuardian::loop, WiFiGuardian::isExitRequested, WiFiGuardian::cleanup)
LEGACY_FACTORY(createLegacySubGhzSentinel, SubSentinel::setup, SubSentinel::loop, SubSentinel::isExitRequested, SubSentinel::cleanup)
LEGACY_FACTORY(createLegacyNrf24Watchdog, GHzWatchdog::setup, GHzWatchdog::loop, GHzWatchdog::isExitRequested, GHzWatchdog::cleanup)
LEGACY_FACTORY(createLegacyFullSpectrum, FullSpectrum::setup, FullSpectrum::loop, FullSpectrum::isExitRequested, FullSpectrum::cleanup)
LEGACY_FACTORY(createLegacySigintKarma, KarmaAttack::setup, KarmaAttack::loop, KarmaAttack::isExitRequested, KarmaAttack::cleanup)
LEGACY_FACTORY(createLegacySigintSavedCaptures, SavedCaptures::setup, SavedCaptures::loop, SavedCaptures::isExitRequested, SavedCaptures::cleanup)
LEGACY_FACTORY(createLegacySigintIotRecon, IotRecon::setup, IotRecon::loop, IotRecon::isExitRequested, IotRecon::cleanup)
LEGACY_FACTORY(createLegacySigintLoot, LootManager::setup, LootManager::loop, LootManager::isExitRequested, LootManager::cleanup)
LEGACY_FACTORY(createLegacySigintEapolCapture, EapolCapture::setup, EapolCapture::loop, EapolCapture::isExitRequested, EapolCapture::cleanup)

#undef LEGACY_FACTORY
