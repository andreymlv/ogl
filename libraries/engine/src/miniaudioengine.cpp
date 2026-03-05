// miniaudio implementation — compiled exactly once here.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "miniaudioengine.h"

#include "audioclip.h"

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

        ma_audio_buffer_config cfg = ma_audio_buffer_config_init(
            ma_format_f32, 1,
            d->m_samples.size(),
            d->m_samples.data(),
            nullptr);
        cfg.sampleRate = static_cast<ma_uint32>(sampleRate);

        if (ma_audio_buffer_init(&cfg, &d->m_buffer) != MA_SUCCESS) {
            return;
        }
        if (ma_sound_init_from_data_source(engine, &d->m_buffer, 0, nullptr, &d->m_sound) != MA_SUCCESS) {
            ma_audio_buffer_uninit(&d->m_buffer);
            return;
        }
        d->m_valid = true;
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
        ma_engine_uninit(&d->m_engine);
    }
}

bool MiniaudioEngine::init()
{
    if (d->m_valid) {
        return true; // idempotent
    }
    if (ma_engine_init(nullptr, &d->m_engine) != MA_SUCCESS) {
        return false;
    }
    d->m_valid = true;
    return true;
}

bool MiniaudioEngine::isValid() const
{
    return d->m_valid;
}

std::unique_ptr<AudioClip> MiniaudioEngine::createSound(std::span<const float> samples, int sampleRate)
{
    if (!d->m_valid) {
        return nullptr;
    }
    auto clip = std::make_unique<MiniaudioClip>(&d->m_engine, samples, sampleRate);
    if (!clip->isValid()) {
        return nullptr;
    }
    return clip;
}

} // namespace engine
