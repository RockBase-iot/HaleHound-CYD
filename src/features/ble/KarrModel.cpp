#include "features/ble/KarrModel.h"

#include "core/FeatureRunner.h"

KarrFraming KarrClassifier::classifyFraming(uint8_t framingByte) {
    switch (framingByte) {
        case 0xBB:
            return KarrFraming::Counter0xBB;
        case 0x55:
            return KarrFraming::Encrypted0x55;
        case 0x00:
            return KarrFraming::Unsupported;
        default:
            return KarrFraming::Standard;
    }
}

KarrAssessment KarrClassifier::assess(const KarrEncounter& encounter) {
    KarrAssessment assessment;
    assessment.framing = classifyFraming(encounter.framingByte);
    assessment.sendsCommands = false;
    assessment.rawEncounterRecorded = true;

    if (encounter.failure != KarrConnectFailure::None) {
        assessment.verdict = KarrVerdict::ConnectFailed;
        assessment.reason = failureLabel(encounter.failure);
        return assessment;
    }

    if (assessment.framing == KarrFraming::Unsupported) {
        // A unit that is present but speaks a framing this build does not know
        // is reported as unsupported framing. Calling it silent would be wrong:
        // it answered, the firmware just cannot read it yet.
        assessment.verdict = KarrVerdict::UnsupportedFraming;
        assessment.reason = "framing not supported by this build";
        return assessment;
    }

    if (!encounter.answered) {
        assessment.verdict = KarrVerdict::ConnectFailed;
        assessment.reason = "unit never answered";
        return assessment;
    }

    if (encounter.proximityKeyRequired) {
        assessment.verdict = KarrVerdict::KeyRequired;
        assessment.reason = "proximity key required before any reply";
        return assessment;
    }

    if (assessment.framing == KarrFraming::Encrypted0x55) {
        // The 0x55 framing is the hardened generation: it answers, but the
        // reply is opaque without the unit key.
        assessment.verdict = encounter.replyStable ? KarrVerdict::Patched : KarrVerdict::KeyRequired;
        assessment.reason = encounter.replyStable ? "0x55 framing accepted, no exploitable reply"
                                                 : "0x55 reply not reproducible";
        return assessment;
    }

    if (assessment.framing == KarrFraming::Counter0xBB) {
        assessment.verdict = encounter.replyStable ? KarrVerdict::Vulnerable : KarrVerdict::Patched;
        assessment.reason = encounter.replyStable ? "0xBB counter reply reproducible"
                                                  : "0xBB counter reply not reproducible";
        return assessment;
    }

    assessment.verdict = encounter.replyStable ? KarrVerdict::Vulnerable : KarrVerdict::Patched;
    assessment.reason = encounter.replyStable ? "standard framing reply reproducible"
                                              : "standard framing reply not reproducible";
    return assessment;
}

const char* KarrClassifier::framingLabel(KarrFraming framing) {
    switch (framing) {
        case KarrFraming::Standard:
            return "Standard";
        case KarrFraming::Counter0xBB:
            return "0xBB Counter";
        case KarrFraming::Encrypted0x55:
            return "0x55 Encrypted";
        case KarrFraming::Unsupported:
        default:
            return "Unsupported";
    }
}

const char* KarrClassifier::verdictLabel(KarrVerdict verdict) {
    switch (verdict) {
        case KarrVerdict::Vulnerable:
            return "VULNERABLE";
        case KarrVerdict::Patched:
            return "PATCHED";
        case KarrVerdict::KeyRequired:
            return "NEEDS KEY";
        case KarrVerdict::UnsupportedFraming:
            return "UNSUPPORTED FRAMING";
        case KarrVerdict::ConnectFailed:
        default:
            return "CONNECT FAILED";
    }
}

const char* KarrClassifier::failureLabel(KarrConnectFailure failure) {
    switch (failure) {
        case KarrConnectFailure::Timeout:
            return "connect timed out";
        case KarrConnectFailure::OutOfRange:
            return "unit moved out of range";
        case KarrConnectFailure::GattError:
            return "GATT exchange failed";
        case KarrConnectFailure::AuthRequired:
            return "unit required authentication";
        case KarrConnectFailure::Rejected:
            return "unit rejected the connection";
        case KarrConnectFailure::RadioBusy:
            return "radio stack busy";
        case KarrConnectFailure::None:
        default:
            return "no failure";
    }
}

namespace {

class KarrFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Completed, "KARR scan ready; assessment is passive"};
    }

    void tick() override {}

    void requestStop(StopReason) override {}

    FeatureResult end() override {
        return {FeatureResultCode::Completed, "KARR stopped"};
    }
};

}  // namespace

IFeature* createKarrFeature() {
    return new KarrFeature();
}
