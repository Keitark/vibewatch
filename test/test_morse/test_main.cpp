#include <unity.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "command_mapper.h"
#include "morse_decoder.h"
#include "morse_trainer.h"
#include "straight_key_input.h"
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
    TEST_ASSERT_EQUAL_STRING(
        ".-", morse::Decoder::encodePattern(
                   'A', morse::DecodeMode::NormalAlphabet).c_str());
    TEST_ASSERT_EQUAL_STRING(
        ".----", morse::Decoder::encodePattern(
                      '1', morse::DecodeMode::NormalAlphabet).c_str());
}

void testCutNumberAliases() {
    const char* patterns[] = {"-", ".-", "..-", "...-", "....-",
                              ".", "-....", "-...", "-..", "-."};
    for (int digit = 0; digit <= 9; ++digit) {
        TEST_ASSERT_EQUAL_CHAR('0' + digit,
                               morse::Decoder::decodeCutNumber(patterns[digit]));
    }
    TEST_ASSERT_EQUAL_CHAR('\0', morse::Decoder::decodeCutNumber("---"));
    TEST_ASSERT_EQUAL_STRING(
        ".-", morse::Decoder::encodePattern(
                   '1', morse::DecodeMode::SimplifiedNumbers).c_str());
}

void testAutomaticCommitAndWordGap() {
    morse::Decoder decoder;
    decoder.keyDown(0);
    decoder.keyUp(90);
    const std::uint32_t unit = decoder.unitMs();
    auto event = decoder.update(90 + unit * 3);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::Character),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_CHAR('5', event.character);
    TEST_ASSERT_FALSE(event.forced);

    event = decoder.update(90 + unit * 7);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::WordGap),
                          static_cast<int>(event.type));

    morse::Decoder numberDecoder;
    std::uint32_t now = 0;
    enterPulse(numberDecoder, now, 90);
    enterPulse(numberDecoder, now, 300);
    event = numberDecoder.update(now + numberDecoder.unitMs() * 3U);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::Character),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_CHAR('1', event.character);
    TEST_ASSERT_FALSE(event.forced);
}

void testDecoderModeControlsAutomaticAndForcedCommit() {
    morse::Decoder decoder;
    std::uint32_t now = 0;
    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    auto event = decoder.forceCommit(now);
    TEST_ASSERT_EQUAL_CHAR('1', event.character);
    TEST_ASSERT_EQUAL_STRING(".-", event.pattern.c_str());
    TEST_ASSERT_TRUE(event.forced);

    decoder.setMode(morse::DecodeMode::NormalAlphabet);
    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    event = decoder.update(now + decoder.unitMs() * 3U);
    TEST_ASSERT_EQUAL_CHAR('A', event.character);
    TEST_ASSERT_EQUAL_STRING(".-", event.pattern.c_str());
    TEST_ASSERT_FALSE(event.forced);

    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    event = decoder.forceCommit(now);
    TEST_ASSERT_EQUAL_CHAR('A', event.character);
    TEST_ASSERT_TRUE(event.forced);
}

void testCharacterIsNotProducedBeforeCommit() {
    morse::Decoder decoder;
    decoder.keyDown(0);
    auto event = decoder.keyUp(90);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::None),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_STRING(".", decoder.pattern().c_str());
    TEST_ASSERT_EQUAL_CHAR('5', decoder.decodePending());

    const std::uint32_t unit = decoder.unitMs();
    event = decoder.update(90 + unit * 3U - 1U);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::None),
                          static_cast<int>(event.type));

    event = decoder.forceCommit(90 + unit * 3U - 1U);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::Character),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_CHAR('5', event.character);
    TEST_ASSERT_TRUE(event.forced);
}

void testPendingPreviewUsesActiveMode() {
    morse::Decoder decoder;
    std::uint32_t now = 0;
    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    TEST_ASSERT_EQUAL_STRING(".-", decoder.pattern().c_str());
    TEST_ASSERT_EQUAL_CHAR('1', decoder.decodePending());

    decoder.clear();
    decoder.setMode(morse::DecodeMode::NormalAlphabet);
    enterPulse(decoder, now, 90);
    enterPulse(decoder, now, 300);
    TEST_ASSERT_EQUAL_STRING(".-", decoder.pattern().c_str());
    TEST_ASSERT_EQUAL_CHAR('A', decoder.decodePending());
}

void testHeldDashPreviewUsesFinalThresholdAndActiveMode() {
    morse::Decoder decoder;
    decoder.keyDown(1000);
    const std::uint32_t dashAt = 1000 + decoder.unitMs() * 2U;

    TEST_ASSERT_EQUAL_STRING("", decoder.previewPattern(dashAt - 1U).c_str());
    TEST_ASSERT_EQUAL_CHAR('\0', decoder.decodePreview(dashAt - 1U));
    TEST_ASSERT_EQUAL_STRING("-", decoder.previewPattern(dashAt).c_str());
    TEST_ASSERT_EQUAL_CHAR('0', decoder.decodePreview(dashAt));
    TEST_ASSERT_TRUE(decoder.pattern().empty());

    decoder.setMode(morse::DecodeMode::NormalAlphabet);
    TEST_ASSERT_EQUAL_CHAR('T', decoder.decodePreview(dashAt));

    const auto event = decoder.keyUp(dashAt + decoder.unitMs());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::EventType::None),
                          static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_STRING("-", decoder.pattern().c_str());
    TEST_ASSERT_EQUAL_CHAR('T', decoder.decodePending());
}

void testTrainerGeneratesProblemsAndScoresWithoutCommands() {
    morse::Trainer trainer;
    trainer.begin(12345U);
    const std::string pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const char first = trainer.target();
    TEST_ASSERT_TRUE(pool.find(first) != std::string::npos);
    const char wrong = first == 'A' ? 'B' : 'A';
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::TrainingAnswer::Retry),
                          static_cast<int>(trainer.submit(wrong)));
    TEST_ASSERT_EQUAL_UINT8(1, trainer.attempts());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::TrainingAnswer::Retry),
                          static_cast<int>(trainer.submit(wrong)));
    TEST_ASSERT_EQUAL_UINT8(2, trainer.attempts());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::TrainingAnswer::Reveal),
                          static_cast<int>(trainer.submit(wrong)));
    TEST_ASSERT_EQUAL_UINT8(3, trainer.attempts());

    trainer.next();
    TEST_ASSERT_TRUE(pool.find(trainer.target()) != std::string::npos);
    TEST_ASSERT_NOT_EQUAL(first, trainer.target());
    TEST_ASSERT_EQUAL_UINT8(0, trainer.attempts());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::TrainingAnswer::Correct),
                          static_cast<int>(trainer.submit(trainer.target())));

    trainer.setMode(morse::TrainingMode::SimplifiedNumbers);
    TEST_ASSERT_TRUE(trainer.target() >= '0' && trainer.target() <= '9');
    TEST_ASSERT_EQUAL_UINT8(0, trainer.attempts());
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
    TEST_ASSERT_EQUAL_STRING("ACT08", morse::mapCommand('X').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT09", morse::mapCommand('P').eventKey);
    TEST_ASSERT_EQUAL_STRING("ACT12", morse::mapCommand('C').eventKey);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::None),
                          static_cast<int>(morse::mapCommand('A').type));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::None),
                          static_cast<int>(morse::mapCommand('N').type));
    TEST_ASSERT_EQUAL_STRING("AG00", morse::mapCommand('1').eventKey);
    TEST_ASSERT_EQUAL_STRING("AG05", morse::mapCommand('6').eventKey);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::MicrophoneToggle),
                          static_cast<int>(morse::mapCommand('M').type));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::CommandType::None),
                          static_cast<int>(morse::mapCommand('Z').type));
}

void testGroveContactDebounce() {
    morse::StraightKeyInput key(8);
    key.reset(false, false, 0);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, true, 1)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, false, 4)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, true, 7)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, true, 14)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::Pressed),
                          static_cast<int>(key.update(false, true, 15)));
    TEST_ASSERT_TRUE(key.grovePressed());

    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, false, 16)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::Released),
                          static_cast<int>(key.update(false, false, 24)));
    TEST_ASSERT_FALSE(key.grovePressed());
}

void testButtonAndGroveMergeWithoutDuplicateEdges() {
    morse::StraightKeyInput key(8);
    key.reset(false, false, 0);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::Pressed),
                          static_cast<int>(key.update(true, false, 1)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(true, true, 2)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(true, true, 10)));
    TEST_ASSERT_TRUE(key.grovePressed());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, true, 11)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::None),
                          static_cast<int>(key.update(false, false, 12)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(morse::KeyEdge::Released),
                          static_cast<int>(key.update(false, false, 20)));
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
    RUN_TEST(testDecoderModeControlsAutomaticAndForcedCommit);
    RUN_TEST(testCharacterIsNotProducedBeforeCommit);
    RUN_TEST(testPendingPreviewUsesActiveMode);
    RUN_TEST(testHeldDashPreviewUsesFinalThresholdAndActiveMode);
    RUN_TEST(testTrainerGeneratesProblemsAndScoresWithoutCommands);
    RUN_TEST(testShortBounceIsIgnored);
    RUN_TEST(testUnitAdaptsWithinLimits);
    RUN_TEST(testCommandMap);
    RUN_TEST(testGroveContactDebounce);
    RUN_TEST(testButtonAndGroveMergeWithoutDuplicateEdges);
    RUN_TEST(testToneDetectorCalibrationAndEdges);
    RUN_TEST(testOutOfBandToneIsRejected);
    RUN_TEST(testBetweenBinToneIsAccepted);
    return UNITY_END();
}
