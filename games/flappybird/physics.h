#pragma once

#include "constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

// ── Bird physics ────────────────────────────────────────────────────────────

inline void updateBird(float &birdY, float &birdVy, float dt)
{
    birdVy += kGravity * dt;
    birdVy = std::max(birdVy, kMaxFallSpeed);
    birdY += birdVy * dt;
}

inline void flap(float &birdVy)
{
    birdVy = kFlapImpulse;
}

inline float birdRotation(float birdVy)
{
    // Map velocity to tilt: rising → +30°, falling → -90°
    if (birdVy > 0.F) {
        return std::min(birdVy / kFlapImpulse * 30.F, 30.F);
    }
    return std::max(birdVy / kMaxFallSpeed * -90.F, -90.F);
}

// ── AABB collision ──────────────────────────────────────────────────────────

struct AABB {
    float x, y, w, h;
};

inline bool checkAABB(const AABB &a, const AABB &b)
{
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

// ── Pipe collision ──────────────────────────────────────────────────────────

struct PipeState {
    float x = 0.F;
    float gapCenterY = 0.F;
    bool scored = false;
};

inline bool checkCollisions(float birdX, float birdY, const std::array<PipeState, kPipeCount> &pipes)
{
    // Shrink bird hitbox slightly for fairness
    const AABB bird{birdX + 3.F, birdY + 3.F, kBirdW - 6.F, kBirdH - 6.F};

    // Floor / ceiling
    if (birdY <= kFloorY || birdY + kBirdH >= kWorldH) {
        return true;
    }

    for (const auto &p : pipes) {
        const float gapTop = p.gapCenterY + kPipeGap / 2.F;
        const float gapBottom = p.gapCenterY - kPipeGap / 2.F;

        // Top pipe AABB
        const AABB topPipe{p.x, gapTop, kPipeW, kPipeH};
        // Bottom pipe AABB
        const AABB bottomPipe{p.x, gapBottom - kPipeH, kPipeW, kPipeH};

        if (checkAABB(bird, topPipe) || checkAABB(bird, bottomPipe)) {
            return true;
        }
    }
    return false;
}

// ── Pipe movement ───────────────────────────────────────────────────────────

inline void updatePipes(std::array<PipeState, kPipeCount> &pipes, float dt, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(kPipeMinGapY, kPipeMaxGapY);

    for (auto &p : pipes) {
        p.x -= kPipeSpeed * dt;
        if (p.x + kPipeW < 0.F) {
            // Find rightmost pipe
            float maxX = 0.F;
            for (const auto &other : pipes) {
                maxX = std::max(maxX, other.x);
            }
            p.x = maxX + kPipeSpacing;
            p.gapCenterY = dist(rng);
            p.scored = false;
        }
    }
}

// ── Scoring ─────────────────────────────────────────────────────────────────

inline int checkScore(float birdX, std::array<PipeState, kPipeCount> &pipes)
{
    int scored = 0;
    const float birdCenter = birdX + kBirdW / 2.F;
    for (auto &p : pipes) {
        if (!p.scored && birdCenter > p.x + kPipeW) {
            p.scored = true;
            ++scored;
        }
    }
    return scored;
}
