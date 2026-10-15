#pragma once

// KARR — SouthWest Dealer Services BLE car alarm detection and vulnerability
// assessment.
//
// Upstream v3.9.5 shipped KARR: scan until a unit shows up, tap it, and the
// module works out which generation it is and whether it is exploitable,
// returning VULNERABLE / PATCHED / needs-a-key. v4.0 added two more framings
// (0xBB counter and 0x55 encrypted) and made the module name an unsupported
// framing instead of calling a live unit silent.
//
// Two properties are enforced in code rather than only in prose, because they
// are the difference between an assessment and an attack:
//
//  * the assessment never transmits - ``sendsCommands()`` is a constant false
//    and ``assess()`` cannot produce a verdict that requires a command;
//  * a raw encounter is always flagged for recording, including when the
//    connect failed, so a unit is never lost to a power-off after a failure.

#include <cstdint>

class IFeature;

enum class KarrFraming : uint8_t {
    Standard,
    Counter0xBB,
    Encrypted0x55,
    Unsupported,
};

enum class KarrVerdict : uint8_t {
    Vulnerable,
    Patched,
    KeyRequired,
    UnsupportedFraming,
    ConnectFailed,
};

enum class KarrConnectFailure : uint8_t {
    None,
    Timeout,
    OutOfRange,
    GattError,
    AuthRequired,
    Rejected,
    RadioBusy,
};

struct KarrEncounter {
    // Framing byte observed in the advertisement or in the first GATT reply.
    uint8_t framingByte = 0;
    // Generation hint derived from the advertisement payload; 0 means unknown.
    uint8_t generationHint = 0;
    // True when the unit answered the first exchange at all. Liveness is not
    // the same as exploitability.
    bool answered = false;
    // True when the reply stayed byte-stable across the assessment window.
    bool replyStable = false;
    // True when the unit demanded a proximity key before replying.
    bool proximityKeyRequired = false;
    KarrConnectFailure failure = KarrConnectFailure::None;
};

struct KarrAssessment {
    KarrFraming framing = KarrFraming::Unsupported;
    KarrVerdict verdict = KarrVerdict::UnsupportedFraming;
    // Always false: this module assesses, it does not command.
    bool sendsCommands = false;
    // True for every encounter, including failures, so the raw record reaches
    // the SD card before anything can power the unit down.
    bool rawEncounterRecorded = true;
    const char* reason = "not assessed";
};

class KarrClassifier {
public:
    static KarrFraming classifyFraming(uint8_t framingByte);
    static KarrAssessment assess(const KarrEncounter& encounter);

    static bool sendsCommands() { return false; }

    static const char* framingLabel(KarrFraming framing);
    static const char* verdictLabel(KarrVerdict verdict);
    static const char* failureLabel(KarrConnectFailure failure);
};

IFeature* createKarrFeature();
