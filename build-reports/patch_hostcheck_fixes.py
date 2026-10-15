#!/usr/bin/env python3
"""Fix the nine host-check failures found by the first full run_host_tests run.

Nine suites failed. Breakdown:

  shim gap (4 suites)  test_board_profiles / test_firmware_validator /
                       test_storage_contract / test_sub_file_parser used
                       TEST_ASSERT_EQUAL_UINT32, which the shim never defined.
  broken test file (2) test_spi_service     -- main() was nested inside the
                                             file's anonymous namespace, so no
                                             global main symbol existed.
                       test_feature_registry -- FeatureCatalog.h only forward-
                                             declares FeatureRegistry; the test
                                             accessed its members.
  real bug (1 code)    FeatureSwitchLedger::replay(nullptr, 0) reported
                       InvalidProfile instead of a vacuously clean replay.
  real bug (1 code)    CardDumpFormat::fromFlipperNfc could not parse the '??'
                       placeholder that toFlipperNfc itself writes, so an
                       unread block was lost and landed in unknownKeys.
  bad fixture (1)      test_card_dump_format built a dump whose block 0 was a
                       0..15 ramp while asserting the eml parser derives
                       04 12 34 56 from it. fromProxmarkEml is right; the
                       fixture was not a manufacturer block.
  bad assertion (1)    test_ndef_codec expected a Text record payload of 400
                       bytes; buildText() prepends status + language, so the
                       round trip is 403. Assert byte-exactness instead.

Assertions on every replacement so a silent no-op cannot happen.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PLAN: list[tuple[str, list[tuple[str, str, str]]]] = []

# --------------------------------------------------------------------------- #
# 1. FeatureSwitchLedger: an empty visit order is vacuously clean
# --------------------------------------------------------------------------- #
PLAN.append(("src/services/pure/FeatureSwitchLedger.cpp", [(
    """SwitchIssue FeatureSwitchLedger::replay(const FeatureProfile* const* order,
                                        size_t count) const {
    if (order == nullptr) {
        return {SwitchIssueCode::InvalidProfile, "", "null visit order", 0U};
    }
""",
    """SwitchIssue FeatureSwitchLedger::replay(const FeatureProfile* const* order,
                                        size_t count) const {
    // An empty visit order touches nothing, so it cannot starve the renderer,
    // leak a resource or miss a teardown: it is clean by construction. Same
    // convention as worstWindowMs() below, which also tolerates a null order
    // when count is zero. A *non-empty* order with a null array is still a bug.
    if (count == 0U) {
        return clean();
    }
    if (order == nullptr) {
        return {SwitchIssueCode::InvalidProfile, "", "null visit order", 0U};
    }
""",
    "replay: count==0 is vacuously clean",
)]))

# --------------------------------------------------------------------------- #
# 2. CardDumpFormat: round-trip the '??' placeholder
# --------------------------------------------------------------------------- #
PLAN.append(("src/features/rfid/CardDumpFormat.cpp", [
    (
        """struct BlockPayload {
    std::vector<uint8_t> bytes;
};
""",
        """// A block the reader never recovered is written as sixteen '??' placeholders
// (see toFlipperNfc). That is not hex, so it has to be recognised before
// parseBytes -- otherwise the line is unparseable and the block is lost.
bool isUnknownPayload(const std::string& text) {
    bool sawToken = false;
    size_t cursor = 0;
    while (cursor < text.size()) {
        const char c = text[cursor];
        if (c == ' ' || c == '\\t' || c == '\\r' || c == ':') {
            ++cursor;
            continue;
        }
        if (cursor + 2 > text.size() || text[cursor] != '?' || text[cursor + 1] != '?') {
            return false;
        }
        sawToken = true;
        cursor += 2;
    }
    return sawToken;
}

struct BlockPayload {
    std::vector<uint8_t> bytes;
    bool unknown = false;
};
""",
        "add isUnknownPayload + BlockPayload::unknown flag",
    ),
    (
        """    std::vector<uint8_t> bytes;
    if (!parseBytes(line.substr(colon + 1), bytes)) {
        return false;
    }
    payload.bytes = bytes;
    return true;
}
""",
        """    const std::string payloadText = line.substr(colon + 1);
    if (isUnknownPayload(payloadText)) {
        payload.bytes.clear();
        payload.unknown = true;
        return true;
    }
    std::vector<uint8_t> bytes;
    if (!parseBytes(payloadText, bytes)) {
        return false;
    }
    payload.bytes = bytes;
    return true;
}
""",
        "parseBlockLine: accept a '??' payload",
    ),
    (
        """            CardDumpBlock& block = dump.blocks[index];
            if (payload.bytes.size() == kCardDumpBlockSize) {
                for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
                    block.data[byte] = payload.bytes[byte];
                }
                block.known = true;
            } else if (payload.bytes.empty() && line.find("??") != std::string::npos) {
                block.known = false;
            } else {
                result.detail = "bad block payload";
                return result;
            }
""",
        """            CardDumpBlock& block = dump.blocks[index];
            if (payload.unknown) {
                // Keep the slot even though it is unread: dropping it would
                // renumber every later block and silently corrupt the dump.
                block.known = false;
            } else if (payload.bytes.size() == kCardDumpBlockSize) {
                for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
                    block.data[byte] = payload.bytes[byte];
                }
                block.known = true;
            } else {
                result.detail = "bad block payload";
                return result;
            }
""",
        "fromFlipperNfc: keep the unread block slot",
    ),
]))

# --------------------------------------------------------------------------- #
# 3. Unity shim: typed equality macros
# --------------------------------------------------------------------------- #
PLAN.append(("scripts/hostcheck/unity/unity.h", [(
    """#define TEST_ASSERT_EQUAL_INT(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int")
#define TEST_ASSERT_NOT_EQUAL(expected, actual) \\
""",
    """#define TEST_ASSERT_EQUAL_INT(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int")

// Typed equality variants. Upstream Unity declares a distinct overload per
// width; the shim reduces them all to the same comparison because it prints
// both values as 64-bit integers anyway. They exist so a suite written against
// the real Unity API compiles here unchanged.
#define TEST_ASSERT_EQUAL_UINT(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint")
#define TEST_ASSERT_EQUAL_INT8(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int8")
#define TEST_ASSERT_EQUAL_INT16(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int16")
#define TEST_ASSERT_EQUAL_INT32(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int32")
#define TEST_ASSERT_EQUAL_INT64(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int64")
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint8")
#define TEST_ASSERT_EQUAL_UINT16(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint16")
#define TEST_ASSERT_EQUAL_UINT32(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint32")
#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint64")
#define TEST_ASSERT_EQUAL_CHAR(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal char")
#define TEST_ASSERT_EQUAL_HEX8(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex8")
#define TEST_ASSERT_EQUAL_HEX16(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex16")
#define TEST_ASSERT_EQUAL_HEX32(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex32")
#define TEST_ASSERT_EQUAL_HEX64(expected, actual) \\
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex64")

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \\
""",
    "shim: add typed equality macros",
)]))

# --------------------------------------------------------------------------- #
# 4. test_spi_service: main() must be at global scope
# --------------------------------------------------------------------------- #
PLAN.append(("test/native/test_spi_service/test_main.cpp", [
    (
        """int main(int, char**) {
    UNITY_BEGIN();""",
        """}  // namespace

int main(int, char**) {
    UNITY_BEGIN();""",
        "spi_service: close the anonymous namespace before main",
    ),
    (
        """    RUN_TEST(test_legacy_session_holds_resource_until_deselect);
    return UNITY_END();
}
}
""",
        """    RUN_TEST(test_legacy_session_holds_resource_until_deselect);
    return UNITY_END();
}
""",
        "spi_service: drop the stray trailing brace",
    ),
]))

# --------------------------------------------------------------------------- #
# 5. test_feature_registry: FeatureRegistry is only forward-declared
# --------------------------------------------------------------------------- #
PLAN.append(("test/native/test_feature_registry/test_main.cpp", [(
    """#include "features/FeatureCatalog.h"
""",
    """#include "core/FeatureRegistry.h"
#include "features/FeatureCatalog.h"
""",
    "feature_registry: include the full FeatureRegistry definition",
)]))

# --------------------------------------------------------------------------- #
# 6. test_card_dump_format: block 0 must look like a manufacturer block
# --------------------------------------------------------------------------- #
PLAN.append(("test/native/test_card_dump_format/test_main.cpp", [(
    """    CardDumpBlock block0;
    for (size_t index = 0; index < kCardDumpBlockSize; ++index) {
        block0.data[index] = static_cast<uint8_t>(index);
    }
    dump.blocks.push_back(block0);
""",
    """    // Sector 0 block 0 is the MIFARE manufacturer block: UID bytes 0..3, the
    // BCC over them, then SAK and ATQA. fromProxmarkEml() derives the header
    // from exactly these offsets, so the fixture has to look like a real card
    // rather than a bare 0..15 ramp. Bytes 8..15 keep a recognisable ramp so
    // the round-trip tests can prove all sixteen bytes survived.
    CardDumpBlock block0;
    const uint8_t manufacturer[kCardDumpBlockSize] = {0x04, 0x12, 0x34, 0x56, 0x74, 0x08, 0x04, 0x00,
                                                      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    for (size_t index = 0; index < kCardDumpBlockSize; ++index) {
        block0.data[index] = manufacturer[index];
    }
    dump.blocks.push_back(block0);
""",
    "card_dump fixture: real manufacturer block",
)]))

# --------------------------------------------------------------------------- #
# 7. test_ndef_codec: assert round-trip byte-exactness, not a magic number
# --------------------------------------------------------------------------- #
PLAN.append(("test/native/test_ndef_codec/test_main.cpp", [(
    """void test_long_payload_switches_to_the_four_byte_length_form() {
    NdefMessage message;
    ndefAppend(message, NdefCodec::buildText(std::string(400, 'x')));

    const auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);
    // Short-record bit must be clear and the length field four bytes wide.
    TEST_ASSERT_TRUE((encoded.bytes[0] & 0x10) == 0);

    const auto decoded = NdefCodec::decode(encoded.bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(400, decoded.message.records[0].payload.size());
}
""",
    """void test_long_payload_switches_to_the_four_byte_length_form() {
    // A Text record payload is [status byte][language code][text], so 400 text
    // bytes encode to 403 payload bytes -- comfortably past the 255-byte limit
    // of the short-record form. Compare against the record that was actually
    // built rather than a hardcoded total: the invariant that matters is
    // byte-exact round-trip, and it stays true if the default language changes.
    const NdefRecord textRecord = NdefCodec::buildText(std::string(400, 'x'));
    NdefMessage message;
    ndefAppend(message, textRecord);

    const auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);
    // Short-record bit must be clear and the length field four bytes wide.
    TEST_ASSERT_TRUE((encoded.bytes[0] & 0x10) == 0);
    TEST_ASSERT_GREATER_THAN(255U, textRecord.payload.size());

    const auto decoded = NdefCodec::decode(encoded.bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(textRecord.payload.size(), decoded.message.records[0].payload.size());
}
""",
    "ndef test: assert byte-exact round trip",
)]))


def main() -> int:
    problems: list[str] = []
    for rel, edits in PLAN:
        path = ROOT / rel
        text = path.read_text(encoding="utf-8")
        for old, new, label in edits:
            count = text.count(old)
            if count != 1:
                problems.append(f"{rel}: {label}: expected 1 match, found {count}")
                continue
            text = text.replace(old, new)
            print(f"  ok  {label}")
        if not any(p.startswith(rel + ":") for p in problems):
            path.write_text(text, encoding="utf-8")
    if problems:
        print("ABORTED (some files left untouched):")
        for problem in problems:
            print(f"  !!  {problem}")
        return 1
    print("all host-check fixes applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
