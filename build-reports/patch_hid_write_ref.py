#!/usr/bin/env python3
"""Give the HID reader client a write target chosen from the GATT properties.

The sweep used to write to the subscribed notify characteristic, which is wrong
in the same way writing to a UART's TX pin would be: what carries the reply and
what accepts a command are different values, and only one of them is chosen by
the repository's own evidence. The property bits *are* discovered data, so
picking the write target from them is grounded -- unlike picking the reply
characteristic, which is X3-unresolved and stays a reported count.

Every replacement is asserted to occur exactly once; a silent miss would leave
the file half-patched, which is worse than failing.
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "features" / "ble" / "HidReaderClient.h"
SOURCE = ROOT / "src" / "features" / "ble" / "HidReaderClient.cpp"

EDITS: list[tuple[pathlib.Path, str, str]] = [
    # ── header ────────────────────────────────────────────────────────────
    (
        HEADER,
        """    uint16_t serviceCount = 0;
    uint16_t characteristicCount = 0;
    uint16_t notifyCharacteristicCount = 0;
    bool subscribed = false;
    bool replyCaptured = false;
    const char* detail = "not started";
""",
        """    uint16_t serviceCount = 0;
    uint16_t characteristicCount = 0;
    uint16_t notifyCharacteristicCount = 0;
    uint16_t writeCharacteristicCount = 0;
    bool subscribed = false;
    // True when a characteristic accepts a write. The reply characteristic is
    // not knowable here (X3), but the write target is: the property bits come
    // from discovery, so they are evidence rather than a guess.
    bool writableFound = false;
    bool replyCaptured = false;
    const char* detail = "not started";
""",
    ),
    (
        HEADER,
        """    uint16_t notifyCharacteristicCount_ = 0;
    BleCharacteristicRef notifyRef_;
""",
        """    uint16_t notifyCharacteristicCount_ = 0;
    // Where a command goes, and where the reply comes back. Two different
    // values: the notify characteristic is chosen by the property bits, the
    // write target likewise, and neither is asserted to be *the* one the
    // reader's protocol names.
    BleCharacteristicRef notifyRef_;
    BleCharacteristicRef writeRef_;
""",
    ),
    # ── source ────────────────────────────────────────────────────────────
    (
        SOURCE,
        """    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;
    return central_.begin();
""",
        """    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;
    notifyRef_ = BleCharacteristicRef{};
    writeRef_ = BleCharacteristicRef{};
    return central_.begin();
""",
    ),
    (
        SOURCE,
        """    replyCaptured_ = false;
    notificationsSeen_ = 0;
    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;

    const HidReaderCandidate& candidate = candidates_[index];
""",
        """    replyCaptured_ = false;
    notificationsSeen_ = 0;
    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;
    notifyRef_ = BleCharacteristicRef{};
    writeRef_ = BleCharacteristicRef{};

    const HidReaderCandidate& candidate = candidates_[index];
""",
    ),
    (
        SOURCE,
        """    bool haveNotify = false;
    for (uint8_t serviceIndex = 0; serviceIndex < peer.serviceCount; ++serviceIndex) {
        const BleDiscoveredService& service = peer.services[serviceIndex];
        for (uint8_t charIndex = 0; charIndex < service.characteristicCount; ++charIndex) {
            const BleDiscoveredCharacteristic& characteristic = service.characteristics[charIndex];
            out.characteristicCount = static_cast<uint16_t>(out.characteristicCount + 1U);
            if ((characteristic.properties & (kBlePropNotify | kBlePropIndicate)) == 0U) {
                continue;
            }
            ++notifyCharacteristicCount_;
            if (haveNotify) {
                continue;
            }
            BleCharacteristicRef ref;
            ref.service = service.uuid;
            ref.characteristic = characteristic.uuid;
            const BleStatus subscribed =
                central_.subscribe(ref, &HidReaderClient::onNotify, this);
            if (subscribed == BleStatus::Ok) {
                notifyRef_ = ref;
                subscribed_ = true;
                haveNotify = true;
                out.subscribed = true;
            }
        }
    }
    out.notifyCharacteristicCount = notifyCharacteristicCount_;
    out.replyCaptured = replyCaptured_;
    out.detail = haveNotify ? "connected; listening for the reader's reply"
                            : "connected; reader offers nothing to notify on";

    if (!haveNotify) {
        // No notify characteristic means no reply can ever be captured, so the
        // transaction cannot complete. Saying so here beats letting the sweep
        // issue commands into a void and time out.
        connectedCandidate_ = index;
        return BleStatus::NoService;
    }
    connectedCandidate_ = index;
    return BleStatus::Ok;
""",
        """    bool haveNotify = false;
    bool haveWrite = false;
    for (uint8_t serviceIndex = 0; serviceIndex < peer.serviceCount; ++serviceIndex) {
        const BleDiscoveredService& service = peer.services[serviceIndex];
        for (uint8_t charIndex = 0; charIndex < service.characteristicCount; ++charIndex) {
            const BleDiscoveredCharacteristic& characteristic = service.characteristics[charIndex];
            out.characteristicCount = static_cast<uint16_t>(out.characteristicCount + 1U);
            BleCharacteristicRef ref;
            ref.service = service.uuid;
            ref.characteristic = characteristic.uuid;

            const bool canWrite =
                (characteristic.properties & (kBlePropWrite | kBlePropWriteNoResponse)) != 0U;
            if (canWrite) {
                ++writeCharacteristicCount_;
                if (!haveWrite) {
                    writeRef_ = ref;
                    haveWrite = true;
                }
            }

            if ((characteristic.properties & (kBlePropNotify | kBlePropIndicate)) == 0U) {
                continue;
            }
            ++notifyCharacteristicCount_;
            if (haveNotify) {
                continue;
            }
            const BleStatus subscribed =
                central_.subscribe(ref, &HidReaderClient::onNotify, this);
            if (subscribed == BleStatus::Ok) {
                notifyRef_ = ref;
                subscribed_ = true;
                haveNotify = true;
                out.subscribed = true;
            }
        }
    }
    out.notifyCharacteristicCount = notifyCharacteristicCount_;
    out.writeCharacteristicCount = writeCharacteristicCount_;
    if (!haveWrite) {
        writeRef_ = BleCharacteristicRef{};
    }
    out.writableFound = haveWrite;
    out.replyCaptured = replyCaptured_;

    if (!haveNotify || !haveWrite) {
        // Either half missing means the transaction cannot complete -- no reply
        // can ever be captured without a subscription, and no command can be
        // sent without a write target. Saying which half is missing here beats
        // letting the sweep issue commands into a void and time out.
        out.detail = !haveNotify
                         ? (haveWrite ? "connected; reader offers nothing to notify on"
                                      : "connected; reader offers no notify or write characteristic")
                         : "connected; reader offers nothing to write to";
        connectedCandidate_ = index;
        return BleStatus::NoService;
    }
    out.detail = "connected; listening for the reader's reply";
    connectedCandidate_ = index;
    return BleStatus::Ok;
""",
    ),
    (
        SOURCE,
        """    if (connectedCandidate_ == 0xFF || !subscribed_) {
        return BleStatus::Disconnected;
    }
""",
        """    if (connectedCandidate_ == 0xFF || !subscribed_) {
        return BleStatus::Disconnected;
    }
    if (!writeRef_.valid()) {
        // Nothing to write to. A sweep that reported success here would be
        // counting writes that never happened.
        return BleStatus::NoService;
    }
""",
    ),
    (
        SOURCE,
        """            const uint8_t* payload = commands + static_cast<std::size_t>(command) * commandLength;
            const BleStatus written =
                central_.write(notifyRef_, payload, commandLength, false);
""",
        """            const uint8_t* payload = commands + static_cast<std::size_t>(command) * commandLength;
            const BleStatus written =
                central_.write(writeRef_, payload, commandLength, false);
""",
    ),
    (
        SOURCE,
        """    if (connectedCandidate_ == 0xFF || !subscribed_) {
        return BleStatus::Disconnected;
    }
    if (!writeRef_.valid()) {""",
        """    if (connectedCandidate_ == 0xFF || !subscribed_) {
        return BleStatus::Disconnected;
    }
    if (!writeRef_.valid()) {""",
    ),
]


def main() -> int:
    failures: list[str] = []
    pending: dict[pathlib.Path, str] = {}
    for path, old, new in EDITS:
        text = pending.get(path)
        if text is None:
            text = path.read_text(encoding="utf-8")
        count = text.count(old)
        if count != 1:
            failures.append(f"{path.name}: expected 1 occurrence, found {count}\n---\n{old[:160]}")
            continue
        pending[path] = text.replace(old, new)
    if failures:
        for failure in failures:
            print("MISS:", failure, file=sys.stderr)
        return 1
    for path, text in pending.items():
        path.write_text(text, encoding="utf-8")
        print("patched", path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
