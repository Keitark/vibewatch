#include <unity.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "command_mapper.h"
#include "morse_decoder.h"
#include "tone_detector.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kPi = 3.14159265358979323846f;

void enterPulse(morse::Decoder& decoder, std::uint32_t& now,
                std::uint32_t downMs, std::uint32_t gapMs = 30) {
    decoder.keyDown(now);
    now += downMs;
    decoder.keyUp(now);
    now += gapMs;
}

void testAlphabetAndDigitsDecode() {
    TEST_ASSERT_EQUAL_CHAR('A', morse::Decoder::decodePattern(".-"));
    TEST_ASSERT_EQUAL_CHAR('F', morse::Decoder::decodePattern("..-."));
    TEST_ASSERT_EQUAL_CHAR('N', morse::Decoder::decodePattern("-."));
    TEST_ASSERT_EQUAL_CHAR('O', morse::Decoder::decodePattern("---"));
    TEST_ASSERT_EQUAL_CHAR('P', morse::Decoder::decodePattern(".--."));
    TEST_ASSERT_EQUAL_CHAR('6', morse::Decoder::decodePattern("-...."));
    TEST_ASSERT_EQUAL_CHAR('\0', morse::Decoder::decodePattern("......"));
}

void testCutNumberAliases() {
    const char* patterns[] = {"-", ".-", "..-", "...-", "....-",
                              ".", "-....", "-...", "-..", "-."};
    for (int digit = 0; digit <= 9; ++digit) {
        TEST_ASSERT_EQUAL_CHAR('0' + digit,
                               morse::Decoder::decodeCutNumber(patterns[digit]));
    }
    TEST_ASSERT_EQUAL_CHAR('\0', morse::Decoder::decodeCutNumber("---"));
}

void testAutomaticCommitAndWordGap() {
    morse::Decoder decoder;
    decoder.keyDown(0);
    decoder.keyUp(90);
    const std::uint32_t unit = decoder.unitMs();
    auto event = decoder.update(90 + unit * 3);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::Character),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_CHAR('E', event.character);
    TEST_ASSERT_FALSE(event.forced);

    event = decoder.update(90 + unit * 7);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::WordGap),
                          static_cast<int>(event.type));
}

void testForcedCommitMarksAlternateChoice() {
    morse::Decoder decoder;
    std::uint32_t now = 0;
    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    const auto event = decoder.forceCommit(now);
    TEST_ASSERT_EQUAL_CHAR('A', event.character);
    TEST_ASSERT_EQUAL_STRING(".-", event.pattern.c_str());
    TEST_ASSERT_TRUE(event.forced);
    TEST_ASSERT_EQUAL_CHAR('1', morse::Decoder::decodeCutNumber(event.pattern));
}

void testShortBounceIsIgnored() {
    morse::Decoder decoder;
    decoder.keyDown(100);
    const auto event = decoder.keyUp(110);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::None),
                          static_cast<int>(event.type));
    TEST_ASSERT_TRUE(decoder.pattern().empty());
}

void testUnitAdaptsWithinLimits() {
    morse::Decoder decoder;
    std::uint32_t now = 0;
    for (int i = 0; i < 8; ++i) {
        enterPulse(decoder, now, 55, 20);
        decoder.clear();
    }
    TEST_ASSERT_GREATER_OR_EQUAL(50, decoder.unitMs());
    TEST_ASSERT_LESS_THAN(80, decoder.unitMs());
}

void testCommandMap() {
    TEST_ASSERT_EQUAL_STRING("ACT06", morse::mapCommand('F').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT07", morse::mapCommand('O').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT08", morse::mapCommand('N').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT09", morse::mapCommand('P').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT12", morse::mapCommand('A').eventKey);
    TEST_ASSERT_EQUAL_STRING("AG00", morse::mapCommand('1').eventKey);
    TEST_ASSERT_EQUAL_STRING("AG05", morse::mapCommand('6').eventKey);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::MicrophoneToggle),
                          static_cast<int>(morse::mapCommand('M').type));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::None),
                          static_cast<int>(morse::mapCommand('Z').type));
}

std::vector<std::int16_t> makeTone(float frequency, float amplitude,
                                   std::uint32_t sampleRate, std::size_t count,
                                   std::size_t offset = 0) {
    std::vector<std::int16_t> samples(count);
    for (std::size_t i = 0; i < count; ++i) {
        const float phase = 2.0f * kPi * frequency * static_cast<float>(i + offset) /
                            static_cast<float>(sampleRate);
        samples[i] = static_cast<std::int16_t>(std::sin(phase) * amplitude);
    }
    return samples;
}

void testToneDetectorCalibrationAndEdges() {
    morse::ToneDetector detector;
    std::vector<std::int16_t> silence(256, 0);
    for (int i = 0; i < 63; ++i) {
        TEST_ASSERT_FALSE(detector.process(silence.data(), silence.size()));
    }
    TEST_ASSERT_TRUE(detector.calibrated());

    for (int i = 0; i < 2; ++i) {
        const auto tone = makeTone(700.0f, 4000.0f, 16000, 256, i * 256);
        detector.process(tone.data(), tone.size());
    }
    TEST_ASSERT_TRUE(detector.tonePresent());

    detector.process(silence.data(), silence.size());
    detector.process(silence.data(), silence.size());
    TEST_ASSERT_FALSE(detector.tonePresent());
}

void testOutOfBandToneIsRejected() {
    morse::ToneDetector detector;
    std::vector<std::int16_t> silence(256, 0);
    for (int i = 0; i < 63; ++i) {
        detector.process(silence.data(), silence.size());
    }
    for (int i = 0; i < 4; ++i) {
        const auto tone = makeTone(200.0f, 5000.0f, 16000, 256, i * 256);
        detector.process(tone.data(), tone.size());
    }
    TEST_ASSERT_FALSE(detector.tonePresent());
}

void testBetweenBinToneIsAccepted() {
    morse::ToneDetector detector;
    std::vector<std::int16_t> silence(256, 0);
    for (int i = 0; i < 63; ++i) {
        detector.process(silence.data(), silence.size());
    }
    for (int i = 0; i < 2; ++i) {
        const auto tone = makeTone(850.0f, 5000.0f, 16000, 256, i * 256);
        detector.process(tone.data(), tone.size());
    }
    TEST_ASSERT_TRUE(detector.tonePresent());
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(testAlphabetAndDigitsDecode);
    RUN_TEST(testCutNumberAliases);
    RUN_TEST(testAutomaticCommitAndWordGap);
    RUN_TEST(testForcedCommitMarksAlternateChoice);
    RUN_TEST(testShortBounceIsIgnored);
    RUN_TEST(testUnitAdaptsWithinLimits);
    RUN_TEST(testCommandMap);
    RUN_TEST(testToneDetectorCalibrationAndEdges);
    RUN_TEST(testOutOfBandToneIsRejected);
    RUN_TEST(testBetweenBinToneIsAccepted);
    return UNITY_END();
}
