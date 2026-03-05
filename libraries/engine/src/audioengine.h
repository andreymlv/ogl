#pragma once

#include "audioclip.h"
#include "engine_export.h"

#include <memory>
#include <span>

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

protected:
    AudioEngine() = default;
};

} // namespace engine
