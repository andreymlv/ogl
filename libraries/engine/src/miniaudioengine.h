#pragma once

#include "audioengine.h"
#include "engine_export.h"

#include <memory>

namespace engine
{

class MiniaudioEnginePrivate;

// miniaudio implementation of AudioEngine.
// All miniaudio headers are confined to miniaudioengine.cpp — consumers of
// this class do not transitively pull in miniaudio types.
class ENGINE_EXPORT MiniaudioEngine : public AudioEngine
{
public:
    MiniaudioEngine();
    ~MiniaudioEngine() override;

    bool init() override;
    [[nodiscard]] bool isValid() const override;

    [[nodiscard]] std::unique_ptr<AudioClip> createSound(std::span<const float> samples, int sampleRate) override;
    [[nodiscard]] std::unique_ptr<AudioClip> createSoundFromFile(std::string_view path) override;
    [[nodiscard]] std::unique_ptr<AudioClip> createSoundFromData(std::span<const unsigned char> data) override;

private:
    std::unique_ptr<MiniaudioEnginePrivate> d;
};

} // namespace engine
