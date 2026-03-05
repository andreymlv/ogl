#pragma once

#include "audioclip.h"
#include "engine_export.h"

#include <QtGlobal>

#include <memory>
#include <span>
#include <string_view>

namespace engine
{

// Abstract audio backend.
// Swap miniaudio → PipeWire / Qt Multimedia / OpenAL by implementing this interface
// and injecting the concrete type — no engine or game code changes required.
//
// Typical usage:
//   auto audio = std::make_unique<MiniaudioEngine>();
//   audio->init();
//   auto clip = audio->createSound(samples, sampleRate);
//   clip->play();
class ENGINE_EXPORT AudioEngine
{
public:
    virtual ~AudioEngine() = default;

    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;

    virtual bool init() = 0;
    [[nodiscard]] virtual bool isValid() const = 0;

    // Create a clip from raw mono float32 PCM samples.
    // Returns nullptr if the engine is not valid or creation fails.
    [[nodiscard]] virtual std::unique_ptr<AudioClip> createSound(std::span<const float> samples, int sampleRate) = 0;

    // Create a clip from a WAV/FLAC/MP3 file on disk.
    // Default returns nullptr — override in concrete backends.
    [[nodiscard]] virtual std::unique_ptr<AudioClip> createSoundFromFile(std::string_view path)
    {
        Q_UNUSED(path)
        return nullptr;
    }

    // Create a clip from in-memory encoded audio data (WAV/FLAC/MP3).
    // Default returns nullptr — override in concrete backends.
    [[nodiscard]] virtual std::unique_ptr<AudioClip> createSoundFromData(std::span<const unsigned char> data)
    {
        Q_UNUSED(data)
        return nullptr;
    }

protected:
    AudioEngine() = default;
};

} // namespace engine
