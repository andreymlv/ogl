#pragma once

#include "constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

// ── AABB ────────────────────────────────────────────────────────────────────

struct AABB {
    float x, y, w, h;
};

inline bool checkAABB(const AABB &a, const AABB &b)
{
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

// ── Brick state ─────────────────────────────────────────────────────────────

struct BrickState {
    bool alive = true;
    int row = 0;
    int col = 0;

    [[nodiscard]] float x() const
    {
        return kBrickAreaLeft + static_cast<float>(col) * (kBrickW + kBrickPadding);
    }

    [[nodiscard]] float y() const
    {
        return kBrickAreaTop - static_cast<float>(kBrickRows - row) * (kBrickH + kBrickPadding);
    }

    [[nodiscard]] AABB aabb() const
    {
        return {x(), y(), kBrickW, kBrickH};
    }
};

// ── Ball state ──────────────────────────────────────────────────────────────

struct BallState {
    float x = kWorldW / 2.F - kBallSize / 2.F;
    float y = kPaddleY + kPaddleH + 2.F;
    float vx = 0.F;
    float vy = 0.F;
    bool launched = false;
};

// ── Collision events ────────────────────────────────────────────────────────

enum class BreakoutEvent : unsigned {
    None = 0,
    WallBounce = 1 << 0,
    CeilingBounce = 1 << 1,
    PaddleBounce = 1 << 2,
    BrickBounce = 1 << 3,
    BallLost = 1 << 4,
};

inline BreakoutEvent operator|(BreakoutEvent a, BreakoutEvent b)
{
    return static_cast<BreakoutEvent>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

inline BreakoutEvent &operator|=(BreakoutEvent &a, BreakoutEvent b)
{
    a = a | b;
    return a;
}

inline bool hasFlag(BreakoutEvent mask, BreakoutEvent flag)
{
    return (static_cast<unsigned>(mask) & static_cast<unsigned>(flag)) != 0;
}

// ── Paddle movement ─────────────────────────────────────────────────────────

inline float movePaddle(float paddleX, float direction, float dt, float paddleW)
{
    const float minX = kWallThickness;
    const float maxX = kWorldW - kWallThickness - paddleW;
    return std::clamp(paddleX + direction * kPaddleSpeed * dt, minX, maxX);
}

// ── Ball-paddle bounce ──────────────────────────────────────────────────────
// The ball's horizontal velocity is influenced by where it hits the paddle:
// hitting the left edge sends it left, hitting the center sends it straight up,
// hitting the right edge sends it right.

inline bool bounceBallOffPaddle(BallState &ball, float paddleX, float paddleW, float speed)
{
    const AABB ballBox{ball.x, ball.y, kBallSize, kBallSize};
    const AABB paddleBox{paddleX, kPaddleY, paddleW, kPaddleH};

    if (!checkAABB(ballBox, paddleBox) || ball.vy > 0.F) {
        return false;
    }

    // Where on the paddle did the ball hit? -1 = left edge, +1 = right edge
    const float ballCenterX = ball.x + kBallSize / 2.F;
    const float paddleCenterX = paddleX + paddleW / 2.F;
    const float hitPos = std::clamp((ballCenterX - paddleCenterX) / (paddleW / 2.F), -1.F, 1.F);

    // Angle between ~150 degrees (far left) and ~30 degrees (far right)
    // hitPos -1 -> angle 150 deg, hitPos 0 -> 90 deg, hitPos 1 -> 30 deg
    static constexpr float kMinAngle = 0.52F; // ~30 degrees in radians
    static constexpr float kMaxAngle = 2.62F; // ~150 degrees in radians
    const float angle = kMaxAngle - (hitPos + 1.F) / 2.F * (kMaxAngle - kMinAngle);

    ball.vx = speed * std::cos(angle);
    ball.vy = speed * std::sin(angle);

    // Push ball above paddle to avoid re-collision
    ball.y = kPaddleY + kPaddleH;

    return true;
}

// ── Full ball simulation step ───────────────────────────────────────────────

struct SimResult {
    BreakoutEvent events = BreakoutEvent::None;
    int scoreGained = 0;
    int bricksDestroyed = 0;
};

inline SimResult simulateBall(BallState &ball, float paddleX, float paddleW, float speed, std::vector<BrickState> &bricks, float dt)
{
    SimResult result;

    if (!ball.launched) {
        return result;
    }

    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    // Left wall bounce
    if (ball.x <= kWallThickness) {
        ball.x = kWallThickness;
        ball.vx = std::abs(ball.vx);
        result.events |= BreakoutEvent::WallBounce;
    }

    // Right wall bounce
    if (ball.x + kBallSize >= kWorldW - kWallThickness) {
        ball.x = kWorldW - kWallThickness - kBallSize;
        ball.vx = -std::abs(ball.vx);
        result.events |= BreakoutEvent::WallBounce;
    }

    // Ceiling bounce
    if (ball.y + kBallSize >= kWorldH - kWallThickness) {
        ball.y = kWorldH - kWallThickness - kBallSize;
        ball.vy = -std::abs(ball.vy);
        result.events |= BreakoutEvent::CeilingBounce;
    }

    // Floor — ball lost
    if (ball.y < 0.F) {
        result.events |= BreakoutEvent::BallLost;
        return result;
    }

    // Paddle collision
    if (bounceBallOffPaddle(ball, paddleX, paddleW, speed)) {
        result.events |= BreakoutEvent::PaddleBounce;
    }

    // Brick collisions — check each alive brick
    const AABB ballBox{ball.x, ball.y, kBallSize, kBallSize};
    for (auto &brick : bricks) {
        if (!brick.alive) {
            continue;
        }

        const AABB brickBox = brick.aabb();
        if (!checkAABB(ballBox, brickBox)) {
            continue;
        }

        brick.alive = false;
        result.events |= BreakoutEvent::BrickBounce;
        result.scoreGained += kRowScore[brick.row];
        ++result.bricksDestroyed;

        // Determine bounce direction based on penetration depths
        const float overlapLeft = (ball.x + kBallSize) - brickBox.x;
        const float overlapRight = (brickBox.x + brickBox.w) - ball.x;
        const float overlapBottom = (ball.y + kBallSize) - brickBox.y;
        const float overlapTop = (brickBox.y + brickBox.h) - ball.y;

        const float minOverlapX = std::min(overlapLeft, overlapRight);
        const float minOverlapY = std::min(overlapBottom, overlapTop);

        if (minOverlapX < minOverlapY) {
            ball.vx = -ball.vx;
        } else {
            ball.vy = -ball.vy;
        }

        // Only bounce off one brick per frame
        break;
    }

    return result;
}

// ── Initialize bricks ───────────────────────────────────────────────────────

inline std::vector<BrickState> buildBricks()
{
    std::vector<BrickState> bricks;
    bricks.reserve(static_cast<std::size_t>(kBrickRows * kBrickCols));
    for (int row = 0; row < kBrickRows; ++row) {
        for (int col = 0; col < kBrickCols; ++col) {
            bricks.push_back({.alive = true, .row = row, .col = col});
        }
    }
    return bricks;
}

// ── Count remaining bricks ──────────────────────────────────────────────────

inline int countAliveBricks(const std::vector<BrickState> &bricks)
{
    int count = 0;
    for (const auto &b : bricks) {
        if (b.alive) {
            ++count;
        }
    }
    return count;
}

// ── Audio helpers — generate simple sine wave samples ────────────────────────

inline std::vector<float> makeSine(float frequency, float duration, float volume = 0.3F)
{
    const auto sampleCount = static_cast<int>(static_cast<float>(kSampleRate) * duration);
    std::vector<float> samples(static_cast<std::size_t>(sampleCount));
    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        // Fade out to avoid click
        const float envelope = 1.F - t / duration;
        samples[static_cast<std::size_t>(i)] = volume * envelope * std::sin(2.F * 3.14159265F * frequency * t);
    }
    return samples;
}
