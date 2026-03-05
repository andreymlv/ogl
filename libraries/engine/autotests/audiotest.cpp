#include <QTest>

#include "audioengine.h"
#include "audioclip.h"
#include "miniaudioengine.h"

class AudioTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    // MiniaudioEngine construction
    void defaultConstructedIsInvalid();

    // init()
    void initReturnsBool();
    void isValidAfterSuccessfulInit();

    // createSound()
    void createSoundReturnsNonNull();
    void createdClipIsValid();

    // play()
    void playDoesNotCrash();
    void playTwiceDoesNotCrash();

private:
    bool m_hasDevice = false;

    // Shared engine for device-dependent tests — initialised in initTestCase.
    std::unique_ptr<engine::AudioEngine> m_audio;
};

// ── helpers ───────────────────────────────────────────────────────────────────

// One second of 440 Hz sine, mono float32.
static std::vector<float> makeSine440()
{
    constexpr int kSampleRate = 44100;
    constexpr float kFreq = 440.0F;
    constexpr float kDuration = 0.05F; // short clip — 50 ms is enough for tests
    const int n = static_cast<int>(kDuration * kSampleRate);
    std::vector<float> buf(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        buf[static_cast<std::size_t>(i)] = 0.2F * std::sin(2.0F * static_cast<float>(M_PI) * kFreq * t);
    }
    return buf;
}

// ── fixture ───────────────────────────────────────────────────────────────────

void AudioTest::initTestCase()
{
    auto audio = std::make_unique<engine::MiniaudioEngine>();
    m_hasDevice = audio->init();
    if (m_hasDevice) {
        m_audio = std::move(audio);
    }
    // If no audio device is present (CI), device-dependent tests will QSKIP.
}

// ── tests ─────────────────────────────────────────────────────────────────────

void AudioTest::defaultConstructedIsInvalid()
{
    engine::MiniaudioEngine audio;
    QVERIFY(!audio.isValid());
}

void AudioTest::initReturnsBool()
{
    engine::MiniaudioEngine audio;
    const bool result = audio.init();
    // Either succeeds (device present) or fails gracefully — must not crash.
    QCOMPARE(audio.isValid(), result);
}

void AudioTest::isValidAfterSuccessfulInit()
{
    if (!m_hasDevice) {
        QSKIP("No audio device available");
    }
    QVERIFY(m_audio->isValid());
}

void AudioTest::createSoundReturnsNonNull()
{
    if (!m_hasDevice) {
        QSKIP("No audio device available");
    }
    const auto samples = makeSine440();
    auto clip = m_audio->createSound(samples, 44100);
    QVERIFY(clip != nullptr);
}

void AudioTest::createdClipIsValid()
{
    if (!m_hasDevice) {
        QSKIP("No audio device available");
    }
    const auto samples = makeSine440();
    auto clip = m_audio->createSound(samples, 44100);
    QVERIFY(clip->isValid());
}

void AudioTest::playDoesNotCrash()
{
    if (!m_hasDevice) {
        QSKIP("No audio device available");
    }
    const auto samples = makeSine440();
    auto clip = m_audio->createSound(samples, 44100);
    clip->play(); // must not crash
    QVERIFY(true);
}

void AudioTest::playTwiceDoesNotCrash()
{
    if (!m_hasDevice) {
        QSKIP("No audio device available");
    }
    const auto samples = makeSine440();
    auto clip = m_audio->createSound(samples, 44100);
    clip->play();
    clip->play(); // restart — must not crash
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(AudioTest)
#include "audiotest.moc"
