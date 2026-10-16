// The Chameleon Ultra frame codec.
//
// Two things make this worth its own suite rather than being folded into the
// client's. The LRC rule is a single arithmetic statement that everything else
// depends on, so it is pinned directly. And the receiver is what turns a byte
// stream with no message boundaries into frames -- the reply arrives over the
// Nordic UART notify characteristic in MTU-sized chunks, so "one notification,
// one message" is the assumption that works on short replies and fails the first
// time a full slot table comes back. The split, mid-stream-start and
// corrupt-then-recover cases are the whole point, so they are tested here.

#include <unity.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "features/ble/ChameleonFrame.h"

namespace {

// Feeds a whole byte range and reports how many frames came out.
struct FeedSummary {
    uint32_t frames = 0;
    uint32_t resyncs = 0;
};

FeedSummary feedAll(ChameleonFrameReceiver& receiver, const uint8_t* bytes, std::size_t length,
                    std::vector<ChameleonFrame>* collected = nullptr) {
    FeedSummary summary;
    for (std::size_t index = 0; index < length; ++index) {
        const ChameleonFrameReceiver::Feed feed = receiver.push(bytes[index]);
        if (feed == ChameleonFrameReceiver::Feed::FrameReady) {
            ++summary.frames;
            if (collected != nullptr) {
                collected->push_back(receiver.frame());
            }
        } else if (feed == ChameleonFrameReceiver::Feed::Resync) {
            ++summary.resyncs;
        }
    }
    // The counters are the receiver's own view of the same stream. They are
    // checked where each one matters rather than here: a helper that returns a
    // value cannot contain an assertion macro, since the macros expand to a bare
    // `return;`.
    return summary;
}

// No TEST_ASSERT_EQUAL_MEMORY in this repository's unity.h, and an assertion
// macro expands to a bare `return;`, so a byte comparison has to happen inside a
// void function.
void expectBytes(const uint8_t* expected, const uint8_t* actual, std::size_t length) {
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(expected, actual, length));
}

}  // namespace

// ─── The LRC rule ──────────────────────────────────────────────────────────

void test_the_lrc_makes_the_byte_sum_wrap_to_zero() {
    // LRC1 is the LRC over the single SOF byte, and the format fixes it at 0xEF.
    // Computing it rather than hardcoding it is what proves the rule.
    const uint8_t sof[1] = {kChameleonSof};
    TEST_ASSERT_EQUAL_HEX8(0xEF, chameleonLrc(sof, 1));
    TEST_ASSERT_EQUAL_HEX8(kChameleonLrc1, chameleonLrc(sof, 1));

    // The empty field has LRC 0, which is what makes a zero-length frame legal.
    TEST_ASSERT_EQUAL_HEX8(0x00, chameleonLrc(nullptr, 0));

    // And the defining property: covered bytes plus their LRC wrap to zero.
    const uint8_t fields[][4] = {
        {0x00, 0x00, 0x00, 0x00},
        {0x03, 0xE8, 0x00, 0x01},
        {0xFF, 0xFF, 0xFF, 0xFF},
        {0x7F, 0x80, 0x01, 0xFE},
    };
    for (const auto& field : fields) {
        uint32_t sum = chameleonLrc(field, 4);
        for (uint8_t byte : field) {
            sum += byte;
        }
        TEST_ASSERT_EQUAL_HEX32(0, sum & 0xFFU);
    }
}

// ─── Encoding ──────────────────────────────────────────────────────────────

void test_a_command_with_no_payload_matches_the_hand_computed_frame() {
    uint8_t frame[32] = {};
    const std::size_t written =
        encodeChameleonFrame(1000, nullptr, 0, frame, sizeof(frame));
    // LEN + 10 with LEN 0, i.e. the bare preamble and its two LRCs.
    TEST_ASSERT_EQUAL_UINT(10, static_cast<unsigned>(written));

    const uint8_t expected[10] = {0x11, 0xEF, 0x03, 0xE8, 0x00, 0x00,
                                  0x00, 0x00, 0x15, 0x00};
    expectBytes(expected, frame, sizeof(expected));
}

void test_a_payload_is_placed_between_the_two_lrcs() {
    const uint8_t payload[1] = {0x02};
    uint8_t frame[32] = {};
    const std::size_t written =
        encodeChameleonFrame(1003, payload, 1, frame, sizeof(frame));
    TEST_ASSERT_EQUAL_UINT(11, static_cast<unsigned>(written));

    const uint8_t expected[11] = {0x11, 0xEF, 0x03, 0xEB, 0x00, 0x00,
                                  0x00, 0x01, 0x11, 0x02, 0xFE};
    expectBytes(expected, frame, sizeof(expected));
}

void test_an_oversized_payload_or_a_short_buffer_is_refused_not_truncated() {
    uint8_t frame[kChameleonMaxFrameBytes] = {};

    // One byte past what this build carries: refused.
    const std::vector<uint8_t> tooBig(kChameleonMaxPayload + 1U, 0xAA);
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(encodeChameleonFrame(
                                  1000, tooBig.data(), static_cast<uint16_t>(tooBig.size()),
                                  frame, sizeof(frame))));

    // Exactly at the limit is accepted, and produces the largest legal frame.
    const std::vector<uint8_t> atLimit(kChameleonMaxPayload, 0xAA);
    TEST_ASSERT_EQUAL_UINT(kChameleonMaxFrameBytes,
                           static_cast<unsigned>(encodeChameleonFrame(
                               1000, atLimit.data(), static_cast<uint16_t>(atLimit.size()),
                               frame, sizeof(frame))));

    // A buffer one byte short of the frame must refuse: a truncated frame on the
    // wire would be rejected by the peer's LRC anyway, so sending it is worse
    // than not sending it.
    uint8_t cramped[kChameleonFrameOverhead] = {};
    const uint8_t one = 0x01;
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(encodeChameleonFrame(1003, &one, 1, cramped,
                                                                        sizeof(cramped))));
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(encodeChameleonFrame(1000, nullptr, 0,
                                                                        nullptr, 16)));
}

// ─── Whole-buffer decode ───────────────────────────────────────────────────

void test_a_frame_round_trips_through_encode_and_decode() {
    const uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t wire[32] = {};
    const std::size_t written = encodeChameleonFrame(1019, payload, 4, wire, sizeof(wire));
    TEST_ASSERT_TRUE(written > 0);

    ChameleonFrame decoded;
    TEST_ASSERT_EQUAL_UINT(static_cast<unsigned>(written),
                           static_cast<unsigned>(decodeChameleonFrame(wire, written, decoded)));
    TEST_ASSERT_EQUAL_HEX16(1019, decoded.command);
    TEST_ASSERT_EQUAL_HEX16(0x0000, decoded.status);
    TEST_ASSERT_TRUE(decoded.statusOk());
    TEST_ASSERT_EQUAL_UINT16(4, decoded.length);
    expectBytes(payload, decoded.data, 4);
}

void test_decode_reports_where_the_frame_ended_and_ignores_what_follows() {
    const uint8_t payload[2] = {0xAA, 0xBB};
    uint8_t wire[64] = {};
    const std::size_t written = encodeChameleonFrame(1019, payload, 2, wire, sizeof(wire));
    // A second frame trails the first; the decode must stop at the boundary.
    const std::size_t second =
        encodeChameleonFrame(1000, nullptr, 0, wire + written, sizeof(wire) - written);
    TEST_ASSERT_TRUE(second > 0);

    ChameleonFrame decoded;
    TEST_ASSERT_EQUAL_UINT(written, static_cast<unsigned>(decodeChameleonFrame(wire, written + second, decoded)));
    TEST_ASSERT_EQUAL_HEX16(1019, decoded.command);
}

void test_decode_reports_nothing_consumed_for_a_partial_or_corrupt_frame() {
    ChameleonFrame decoded;

    // Truncated mid-header.
    const uint8_t partial[5] = {kChameleonSof, kChameleonLrc1, 0x03, 0xE8, 0x00};
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(decodeChameleonFrame(partial, sizeof(partial), decoded)));

    // Right shape, wrong LRC2.
    const uint8_t badLrc[10] = {0x11, 0xEF, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(decodeChameleonFrame(badLrc, sizeof(badLrc), decoded)));

    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(decodeChameleonFrame(nullptr, 10, decoded)));
}

// ─── Stream reassembly ─────────────────────────────────────────────────────

void test_a_frame_split_across_notifications_is_reassembled() {
    const uint8_t payload[6] = {0x11, 0xEF, 0x00, 0x01, 0x02, 0x03};
    uint8_t wire[32] = {};
    const std::size_t written = encodeChameleonFrame(2000, payload, 6, wire, sizeof(wire));
    TEST_ASSERT_TRUE(written > kChameleonFrameOverhead);

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;

    // Deliberately cut at 3 bytes and again at 9: the boundaries fall inside the
    // header and inside the data, which is where a naive reassembler breaks.
    const std::size_t cuts[] = {3, 9, written};
    std::size_t cursor = 0;
    for (std::size_t cut : cuts) {
        const FeedSummary summary =
            feedAll(receiver, wire + cursor, cut - cursor, &collected);
        // Only the last chunk can complete the frame.
        if (cut == written) {
            TEST_ASSERT_EQUAL_UINT32(1, summary.frames);
        } else {
            TEST_ASSERT_EQUAL_UINT32(0, summary.frames);
        }
        cursor = cut;
    }

    TEST_ASSERT_EQUAL(1, static_cast<int>(collected.size()));
    TEST_ASSERT_EQUAL_HEX16(2000, collected[0].command);
    TEST_ASSERT_EQUAL_UINT16(6, collected[0].length);
    expectBytes(payload, collected[0].data, 6);
    TEST_ASSERT_EQUAL_UINT32(0, receiver.lrcFailures());
    TEST_ASSERT_EQUAL_UINT32(0, receiver.resyncs());
}

void test_a_frame_starting_mid_stream_is_found() {
    const uint8_t payload[2] = {0xDE, 0xAD};
    uint8_t wire[32] = {};
    const std::size_t written = encodeChameleonFrame(1019, payload, 2, wire, sizeof(wire));

    // Garbage before the frame, the way a link that just came up looks.
    std::vector<uint8_t> stream = {0x00, 0xFF, 0x11, 0x00, 0x42, 0x11, 0x11, 0xEF};
    // That trailing 0xEF is a coincidental start; splice the real frame after a
    // few more noise bytes so the hunt is genuine.
    stream.push_back(0x7A);
    stream.insert(stream.end(), wire, wire + written);

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    const FeedSummary summary = feedAll(receiver, stream.data(), stream.size(), &collected);

    TEST_ASSERT_EQUAL_UINT32(1, summary.frames);
    TEST_ASSERT_EQUAL(1, static_cast<int>(collected.size()));
    TEST_ASSERT_EQUAL_HEX16(1019, collected[0].command);
    TEST_ASSERT_EQUAL_UINT16(2, collected[0].length);
    expectBytes(payload, collected[0].data, 2);
}

void test_two_frames_back_to_back_are_both_decoded() {
    uint8_t wire[64] = {};
    std::size_t cursor = 0;
    cursor += encodeChameleonFrame(1019, nullptr, 0, wire + cursor, sizeof(wire) - cursor);
    const uint8_t payload[3] = {0x01, 0x02, 0x03};
    cursor += encodeChameleonFrame(1012, payload, 3, wire + cursor, sizeof(wire) - cursor);

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    const FeedSummary summary = feedAll(receiver, wire, cursor, &collected);

    TEST_ASSERT_EQUAL_UINT32(2, summary.frames);
    TEST_ASSERT_EQUAL_HEX16(1019, collected[0].command);
    TEST_ASSERT_EQUAL_UINT16(0, collected[0].length);
    TEST_ASSERT_EQUAL_HEX16(1012, collected[1].command);
    TEST_ASSERT_EQUAL_UINT16(3, collected[1].length);
}

void test_a_zero_length_frame_completes_on_its_lrc3() {
    // A reply with no data still carries the postamble, so the frame is ten bytes
    // and the last byte is the LRC over an empty field.
    uint8_t wire[16] = {};
    const std::size_t written = encodeChameleonFrame(1013, nullptr, 0, wire, sizeof(wire));
    TEST_ASSERT_EQUAL_UINT(10, static_cast<unsigned>(written));

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    // Feed one byte short: nothing must be emitted until the postamble lands.
    const FeedSummary shortFeed =
        feedAll(receiver, wire, 9, &collected);
    TEST_ASSERT_EQUAL_UINT32(0, shortFeed.frames);

    const FeedSummary full = feedAll(receiver, wire + 9, 1, &collected);
    TEST_ASSERT_EQUAL_UINT32(1, full.frames);
}

// ─── Corruption ────────────────────────────────────────────────────────────

void test_a_corrupt_payload_is_rejected_and_the_stream_recovers() {
    const uint8_t payload[4] = {0x10, 0x20, 0x30, 0x40};
    uint8_t bad[32] = {};
    const std::size_t length = encodeChameleonFrame(1019, payload, 4, bad, sizeof(bad));
    // Flip one data byte, leaving the header and both LRCs intact: the header's
    // LRC2 still passes, so only the data check can catch this.
    bad[10] ^= 0x01;

    uint8_t good[32] = {};
    const std::size_t goodLength = encodeChameleonFrame(1019, payload, 4, good, sizeof(good));

    std::vector<uint8_t> stream(bad, bad + length);
    stream.insert(stream.end(), good, good + goodLength);

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    const FeedSummary summary = feedAll(receiver, stream.data(), stream.size(), &collected);

    // One frame lost, one recovered: a corrupt frame must not swallow the next.
    TEST_ASSERT_EQUAL_UINT32(1, summary.frames);
    TEST_ASSERT_EQUAL_UINT32(1, receiver.lrcFailures());
    TEST_ASSERT_EQUAL_UINT32(1, summary.resyncs);
    expectBytes(payload, collected[0].data, 4);
}

void test_a_corrupt_header_is_rejected_without_reading_past_the_buffer() {
    // CMD 1000, STATUS 0, LEN 0xFFFF with a *correct* LRC2: the payload length is
    // well formed as far as the LRC is concerned, so only the cap stands between
    // this and a 64 KB read into a 512-byte buffer.
    const uint8_t hostile[9] = {0x11, 0xEF, 0x03, 0xE8, 0x00, 0x00, 0xFF, 0xFF, 0x17};
    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    const FeedSummary summary = feedAll(receiver, hostile, sizeof(hostile), &collected);

    TEST_ASSERT_EQUAL_UINT32(0, summary.frames);
    TEST_ASSERT_EQUAL_UINT32(1, receiver.oversizedFrames());
    TEST_ASSERT_EQUAL_UINT32(1, summary.resyncs);
    // Nothing was decoded, so no frame was handed out carrying a bogus length.
    TEST_ASSERT_EQUAL(0, static_cast<int>(collected.size()));
}

void test_an_oversized_frame_does_not_stop_the_next_valid_one() {
    const uint8_t hostile[9] = {0x11, 0xEF, 0x03, 0xE8, 0x00, 0x00, 0xFF, 0xFF, 0x17};
    const uint8_t payload[2] = {0xCA, 0xFE};
    uint8_t good[32] = {};
    const std::size_t goodLength = encodeChameleonFrame(1019, payload, 2, good, sizeof(good));

    std::vector<uint8_t> stream(hostile, hostile + sizeof(hostile));
    stream.insert(stream.end(), good, good + goodLength);

    ChameleonFrameReceiver receiver;
    std::vector<ChameleonFrame> collected;
    const FeedSummary summary = feedAll(receiver, stream.data(), stream.size(), &collected);

    TEST_ASSERT_EQUAL_UINT32(1, summary.frames);
    TEST_ASSERT_EQUAL_HEX16(1019, collected[0].command);
    expectBytes(payload, collected[0].data, 2);
}

void test_a_lone_start_byte_is_not_treated_as_a_frame() {
    ChameleonFrameReceiver receiver;
    const uint8_t noise[4] = {kChameleonSof, kChameleonSof, 0x00, kChameleonSof};
    const FeedSummary summary = feedAll(receiver, noise, sizeof(noise));

    TEST_ASSERT_EQUAL_UINT32(0, summary.frames);
    // A false start is reported, not swallowed: "the peer sent something odd" is
    // information, and a silent drop is not.
    TEST_ASSERT_TRUE(summary.resyncs > 0);
}

// ─── Status ────────────────────────────────────────────────────────────────

void test_only_the_status_this_build_can_name_gets_a_name() {
    TEST_ASSERT_TRUE(chameleonStatusOk(0x0000));
    TEST_ASSERT_EQUAL_STRING("OK", chameleonStatusLabel(0x0000));

    TEST_ASSERT_FALSE(chameleonStatusOk(0x0001));
    // The firmware's status table was not in the evidence set, so a failure is
    // labelled as a failure and its raw code printed -- never given a name that
    // would read as verified.
    TEST_ASSERT_EQUAL_STRING("fail", chameleonStatusLabel(0x0002));

    char buffer[24] = {};
    TEST_ASSERT_EQUAL_UINT(2, static_cast<unsigned>(formatChameleonStatus(0x0000, buffer, sizeof(buffer))));
    TEST_ASSERT_EQUAL_STRING("OK", buffer);
    formatChameleonStatus(0x0002, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("fail 0x0002", buffer);
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(formatChameleonStatus(0, nullptr, 16)));
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(formatChameleonStatus(0, buffer, 0)));
}

void test_a_failed_response_still_parses_as_a_frame() {
    // A handler that refused: the frame is well formed and its status says so.
    // Treating it as a transport failure would report a device answer as a dead
    // link.
    uint8_t wire[32] = {};
    std::size_t written = encodeChameleonFrame(1008, nullptr, 0, wire, sizeof(wire));
    // Rewrite the status in place and fix LRC2, the way the firmware would.
    wire[4] = 0x00;
    wire[5] = 0x02;
    wire[8] = chameleonLrc(wire + 2, 6);

    ChameleonFrame decoded;
    TEST_ASSERT_EQUAL_UINT(written, static_cast<unsigned>(decodeChameleonFrame(wire, written, decoded)));
    TEST_ASSERT_EQUAL_HEX16(1008, decoded.command);
    TEST_ASSERT_FALSE(decoded.statusOk());
    TEST_ASSERT_EQUAL_HEX16(0x0002, decoded.status);
    TEST_ASSERT_EQUAL_STRING("fail", chameleonStatusLabel(decoded.status));
}

// ─── The big-endian accessor ───────────────────────────────────────────────

void test_a_big_endian_field_is_read_only_when_it_fits() {
    const uint8_t payload[4] = {0x12, 0x34, 0xAB, 0xCD};
    uint8_t wire[32] = {};
    const std::size_t written = encodeChameleonFrame(1019, payload, 4, wire, sizeof(wire));

    ChameleonFrame decoded;
    TEST_ASSERT_TRUE(decodeChameleonFrame(wire, written, decoded) > 0);
    TEST_ASSERT_EQUAL_HEX16(0x1234, decoded.dataU16(0));
    TEST_ASSERT_EQUAL_HEX16(0xABCD, decoded.dataU16(2));
    // Past the end it reports zero rather than reading a neighbouring frame.
    TEST_ASSERT_EQUAL_HEX16(0x0000, decoded.dataU16(3));
    TEST_ASSERT_EQUAL_HEX16(0x0000, decoded.dataU16(4));
    TEST_ASSERT_EQUAL_HEX16(0x0000, decoded.dataU16(500));
}

void test_reset_clears_the_counters_and_the_partial_frame() {
    const uint8_t partial[5] = {kChameleonSof, kChameleonLrc1, 0x03, 0xE8, 0x00};
    ChameleonFrameReceiver receiver;
    feedAll(receiver, partial, sizeof(partial));
    TEST_ASSERT_EQUAL_UINT32(0, receiver.framesDecoded());

    receiver.reset();
    TEST_ASSERT_EQUAL_UINT32(0, receiver.framesDecoded());
    TEST_ASSERT_EQUAL_UINT32(0, receiver.lrcFailures());
    TEST_ASSERT_EQUAL_UINT32(0, receiver.resyncs());
    TEST_ASSERT_EQUAL_UINT32(0, receiver.oversizedFrames());

    // And the receiver is back in the hunt: a whole frame from the start lands.
    uint8_t wire[32] = {};
    const std::size_t written = encodeChameleonFrame(1000, nullptr, 0, wire, sizeof(wire));
    std::vector<ChameleonFrame> collected;
    TEST_ASSERT_EQUAL_UINT32(1, feedAll(receiver, wire, written, &collected).frames);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_the_lrc_makes_the_byte_sum_wrap_to_zero);
    RUN_TEST(test_a_command_with_no_payload_matches_the_hand_computed_frame);
    RUN_TEST(test_a_payload_is_placed_between_the_two_lrcs);
    RUN_TEST(test_an_oversized_payload_or_a_short_buffer_is_refused_not_truncated);
    RUN_TEST(test_a_frame_round_trips_through_encode_and_decode);
    RUN_TEST(test_decode_reports_where_the_frame_ended_and_ignores_what_follows);
    RUN_TEST(test_decode_reports_nothing_consumed_for_a_partial_or_corrupt_frame);
    RUN_TEST(test_a_frame_split_across_notifications_is_reassembled);
    RUN_TEST(test_a_frame_starting_mid_stream_is_found);
    RUN_TEST(test_two_frames_back_to_back_are_both_decoded);
    RUN_TEST(test_a_zero_length_frame_completes_on_its_lrc3);
    RUN_TEST(test_a_corrupt_payload_is_rejected_and_the_stream_recovers);
    RUN_TEST(test_a_corrupt_header_is_rejected_without_reading_past_the_buffer);
    RUN_TEST(test_an_oversized_frame_does_not_stop_the_next_valid_one);
    RUN_TEST(test_a_lone_start_byte_is_not_treated_as_a_frame);
    RUN_TEST(test_only_the_status_this_build_can_name_gets_a_name);
    RUN_TEST(test_a_failed_response_still_parses_as_a_frame);
    RUN_TEST(test_a_big_endian_field_is_read_only_when_it_fits);
    RUN_TEST(test_reset_clears_the_counters_and_the_partial_frame);
    return UNITY_END();
}
