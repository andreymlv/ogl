#pragma once

#include "engine_export.h"

namespace engine
{

// Abstract handle for a single playable sound clip.
// Created by AudioEngine::createSound().
// play() always restarts from the beginning — safe to call while already playing.
class ENGINE_EXPORT AudioClip
{
public:
    virtual ~AudioClip() = default;

    AudioClip(const AudioClip &) = delete;
    AudioClip &operator=(const AudioClip &) = delete;

    virtual void play() = 0;
    [[nodiscard]] virtual bool isValid() const = 0;

protected:
    AudioClip() = default;
};

} // namespace engine
