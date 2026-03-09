// miniaudio implementation — compiled exactly once here.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "miniaudioengine.h"

#include "audioclip.h"

#include <QLoggingCategory>

#include "engine_logging.h"

#include <string>
#include <vector>

namespace engine
{

// ── MiniaudioClip (private to this translation unit) ──────────────────────────

class MiniaudioClipPrivate
{
public:
    std::vector<float> m_samples; // keeps the PCM data alive for the duration
    ma_audio_buffer m_buffer{};
    ma_sound m_sound{};
    bool m_valid = false;
};

class MiniaudioClip : public AudioClip
{
public:
    explicit MiniaudioClip(ma_engine *engine, std::span<const float> samples, int sampleRate)
        : d(std::make_unique<MiniaudioClipPrivate>())
    {
        d->m_samples.assign(samples.begin(), samples.end());

        ma_audio_buffer_config cfg = ma_audio_buffer_config_init(ma_format_f32, 1, d->m_samples.size(), d->m_samples.data(), nullptr);
        cfg.sampleRate = static_cast<ma_uint32>(sampleRate);

        if (ma_audio_buffer_init(&cfg, &d->m_buffer) != MA_SUCCESS) {
            qCWarning(Engine) << "Failed to initialize audio buffer";
            return;
        }
        if (ma_sound_init_from_data_source(engine, &d->m_buffer, 0, nullptr, &d->m_sound) != MA_SUCCESS) {
            qCWarning(Engine) << "Failed to create sound from data source";
            ma_audio_buffer_uninit(&d->m_buffer);
            return;
        }
        d->m_valid = true;
        qCDebug(Engine) << "Audio clip created:" << samples.size() << "samples," << sampleRate << "Hz";
    }

    ~MiniaudioClip() override
    {
        if (!d->m_valid) {
            return;
        }
        ma_sound_uninit(&d->m_sound);
        ma_audio_buffer_uninit(&d->m_buffer);
    }

    void play() override
    {
        if (!d->m_valid) {
            return;
        }
        ma_sound_seek_to_pcm_frame(&d->m_sound, 0);
        ma_sound_start(&d->m_sound);
    }

    bool isValid() const override
    {
        return d->m_valid;
    }

private:
    std::unique_ptr<MiniaudioClipPrivate> d;
};

// ── MiniaudioEnginePrivate ────────────────────────────────────────────────────

class MiniaudioEnginePrivate
{
public:
    ma_engine m_engine{};
    bool m_valid = false;
};

// ── MiniaudioEngine ───────────────────────────────────────────────────────────

MiniaudioEngine::MiniaudioEngine()
    : d(std::make_unique<MiniaudioEnginePrivate>())
{
}

MiniaudioEngine::~MiniaudioEngine()
{
    if (d->m_valid) {
        qCDebug(Engine) << "Shutting down miniaudio engine";
        ma_engine_uninit(&d->m_engine);
    }
}

bool MiniaudioEngine::init()
{
    if (d->m_valid) {
        return true; // idempotent
    }
    if (ma_engine_init(nullptr, &d->m_engine) != MA_SUCCESS) {
        qCWarning(Engine) << "Failed to initialize miniaudio engine";
        return false;
    }
    d->m_valid = true;
    qCDebug(Engine) << "Miniaudio engine initialized";
    return true;
}

bool MiniaudioEngine::isValid() const
{
    return d->m_valid;
}

std::unique_ptr<AudioClip> MiniaudioEngine::createSound(std::span<const float> samples, int sampleRate)
{
    if (!d->m_valid) {
        qCWarning(Engine) << "Cannot create sound: audio engine not initialized";
        return nullptr;
    }
    auto clip = std::make_unique<MiniaudioClip>(&d->m_engine, samples, sampleRate);
    if (!clip->isValid()) {
        qCWarning(Engine) << "Failed to create audio clip from PCM data";
        return nullptr;
    }
    return clip;
}

std::unique_ptr<AudioClip> MiniaudioEngine::createSoundFromFile(std::string_view path)
{
    if (!d->m_valid) {
        return nullptr;
    }

    ma_decoder decoder;
    ma_decoder_config decoderCfg = ma_decoder_config_init(ma_format_f32, 1, 44100);
    if (ma_decoder_init_file(std::string(path).c_str(), &decoderCfg, &decoder) != MA_SUCCESS) {
        qCWarning(Engine) << "Failed to decode audio file:" << path.data();
        return nullptr;
    }

    ma_uint64 frameCount = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (frameCount == 0) {
        qCWarning(Engine) << "Audio file has zero frames:" << path.data();
        ma_decoder_uninit(&decoder);
        return nullptr;
    }

    std::vector<float> samples(frameCount);
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, samples.data(), frameCount, &framesRead);
    samples.resize(framesRead);

    const auto sampleRate = static_cast<int>(decoder.outputSampleRate);
    ma_decoder_uninit(&decoder);

    qCDebug(Engine) << "Sound loaded from file:" << path.data() << "(" << framesRead << "frames," << sampleRate << "Hz)";
    return createSound(samples, sampleRate);
}

std::unique_ptr<AudioClip> MiniaudioEngine::createSoundFromData(std::span<const unsigned char> data)
{
    if (!d->m_valid) {
        return nullptr;
    }

    ma_decoder decoder;
    ma_decoder_config decoderCfg = ma_decoder_config_init(ma_format_f32, 1, 44100);
    if (ma_decoder_init_memory(data.data(), data.size(), &decoderCfg, &decoder) != MA_SUCCESS) {
        qCWarning(Engine) << "Failed to decode audio from memory (" << data.size() << "bytes)";
        return nullptr;
    }

    ma_uint64 frameCount = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (frameCount == 0) {
        qCWarning(Engine) << "Audio data from memory has zero frames";
        ma_decoder_uninit(&decoder);
        return nullptr;
    }

    std::vector<float> samples(frameCount);
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, samples.data(), frameCount, &framesRead);
    samples.resize(framesRead);

    const auto sampleRate = static_cast<int>(decoder.outputSampleRate);
    ma_decoder_uninit(&decoder);

    qCDebug(Engine) << "Sound decoded from memory:" << framesRead << "frames," << sampleRate << "Hz";
    return createSound(samples, sampleRate);
}

} // namespace engine
