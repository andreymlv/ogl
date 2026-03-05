#pragma once

#include "constants.h"
#include "physics.h"

#include <array>

// Find the next pipe the bird hasn't passed yet.
// Returns the gap center Y, or kBirdStartY if no pipe ahead.
inline float aiTargetY(const std::array<PipeState, kPipeCount> &pipes)
{
    const float birdRight = kBirdX + kBirdW;
    float closestX = kWorldW + kPipeSpacing; // fallback: far right
    float targetY = kBirdStartY;
    for (const auto &p : pipes) {
        const float pipeRight = p.x + kPipeW;
        if (pipeRight > birdRight && p.x < closestX) {
            closestX = p.x;
            targetY = p.gapCenterY;
        }
    }
    return targetY;
}

// Returns true if the AI wants to flap this frame.
inline bool aiShouldFlap(float birdY, float birdVy,
                         const std::array<PipeState, kPipeCount> &pipes)
{
    // Aim below the gap center. Each flap produces ~49px of rise
    // (kFlapImpulse² / (2 * |kGravity|)), so the bird will peak ~49px
    // above where it flapped. Aiming 25px low keeps the peak ~24px
    // above center — safely inside the 100px gap on both sides.
    constexpr float kAimOffset = 25.F;
    const float target = aiTargetY(pipes) - kAimOffset;
    const float birdCenter = birdY + kBirdH / 2.F;

    // Flap when below the adjusted target and not rising fast.
    // The velocity cap (50) allows chaining flaps for fast climbs
    // while still preventing rapid over-flapping near the target.
    return birdCenter < target && birdVy < 50.F;
}
