#pragma once

// RFID back end selection.
//
// Upstream v3.8.0 turned RFID into a back-end picker (PN-532, Chameleon Ultra,
// ST25R3916) so the same NFC toolset runs on whichever reader is attached.
// v4.0 landed the ST25R3916 driver and rebuilt the module into seven screens.
//
// The capability matrix below is what the UI uses to grey out actions a given
// back end cannot perform. It describes the reader silicon and the driver in
// this repository; it is not a claim about a specific card in the field.

#include <cstdint>

class IFeature;

enum class RfidBackend : uint8_t {
    Pn532,
    ChameleonUltra,
    St25r3916,
};

enum class RfidScreen : uint8_t {
    BackendPicker,
    CardScanner,
    CardReader,
    CardDump,
    KeyCheck,
    CardWrite,
    Guide,
};

struct RfidBackendCapability {
    bool mifareClassic;
    bool mifareUltralight;
    bool ntag;
    bool isoDep;
    bool iclass;
    bool magicGen1A;
    bool nestedAuthentication;
    bool keyedWrite;
    bool flipperNfcExport;
    bool proxmarkEmlExport;
    bool hfEmulation;
    bool lfSupport;
    bool mfkey32NonceCapture;
};

class RfidBackendModel {
public:
    static constexpr uint8_t kScreenCount = 7;

    static RfidBackendCapability capability(RfidBackend backend);
    static const char* backendLabel(RfidBackend backend);
    static uint8_t backendCount();
    static RfidBackend backendAt(uint8_t index);

    static RfidScreen screenAt(uint8_t index);
    static const char* screenLabel(RfidScreen screen);
    static bool screenAvailable(RfidBackend backend, RfidScreen screen);

    // True when the backend can produce a real Flipper .nfc / Proxmark .eml
    // pair for the given dump kind.
    static bool canExportDump(RfidBackend backend, bool isMifareClassic);
};

IFeature* createRfidBackendFeature();
