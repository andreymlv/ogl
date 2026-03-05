#pragma once

#include "constants.h"
#include "network.h"

#include <algorithm>
#include <cmath>

// ── Collision events (flags) ────────────────────────────────────────────────

enum class HitEvent : quint8 {
    None = 0,
    Wall = 1 << 0,
    Paddle = 1 << 1,
    ScoreLeft = 1 << 2,
    ScoreRight = 1 << 3,
};

inline HitEvent operator|(HitEvent a, HitEvent b)
{
    return static_cast<HitEvent>(static_cast<quint8>(a) | static_cast<quint8>(b));
}

inline HitEvent &operator|=(HitEvent &a, HitEvent b)
{
    a = a | b;
    return a;
}

inline bool hasFlag(HitEvent mask, HitEvent flag)
{
    return (static_cast<quint8>(mask) & static_cast<quint8>(flag)) != 0;
}

// ── Physics free functions ──────────────────────────────────────────────────

inline float movePaddle(float y, Direction dir, float dt)
{
    const float step = static_cast<float>(static_cast<qint8>(dir)) * kPaddleSpeed * dt;
    return std::clamp(y + step, -1.0F + kPaddleH / 2.0F, 1.0F - kPaddleH / 2.0F);
}

inline void resetBall(GameState &state, float directionX)
{
    state.m_ball = {
        .m_x = 0.0F,
        .m_y = 0.0F,
        .m_vx = directionX * kBallSpeedInitial,
        .m_vy = kBallSpeedInitial * 0.5F,
    };
}

inline bool checkPaddleCollision(Ball &b, float paddleX, float paddleY, float xSign)
{
    const bool movingToward = (xSign > 0.0F) ? (b.m_vx < 0.0F) : (b.m_vx > 0.0F);
    if (!movingToward) {
        return false;
    }

    const bool xOverlap = b.m_x - kBallSize / 2.0F <= paddleX + kPaddleW && b.m_x + kBallSize / 2.0F >= paddleX;
    const bool yOverlap = std::abs(b.m_y - paddleY) < (kPaddleH + kBallSize) / 2.0F;

    if (!xOverlap || !yOverlap) {
        return false;
    }

    const float speed = std::min(std::abs(b.m_vx) * kBallSpeedIncrease, kBallSpeedMax);
    b.m_vx = xSign * speed;
    b.m_vy = ((b.m_y - paddleY) / (kPaddleH / 2.0F)) * speed * 0.75F;
    return true;
}

inline HitEvent simulateBall(GameState &state, float dt)
{
    HitEvent events = HitEvent::None;
    Ball &b = state.m_ball;
    b.m_x += b.m_vx * dt;
    b.m_y += b.m_vy * dt;

    // Wall bounce
    if (b.m_y + kBallSize / 2.0F >= 1.0F || b.m_y - kBallSize / 2.0F <= -1.0F) {
        b.m_vy = -b.m_vy;
        b.m_y = std::clamp(b.m_y, -1.0F + kBallSize / 2.0F, 1.0F - kBallSize / 2.0F);
        events |= HitEvent::Wall;
    }

    // Paddle collisions
    if (checkPaddleCollision(b, kLeftPaddleX, state.m_p1Y, 1.0F)) {
        events |= HitEvent::Paddle;
    }
    if (checkPaddleCollision(b, kRightPaddleX, state.m_p2Y, -1.0F)) {
        events |= HitEvent::Paddle;
    }

    // Scoring
    if (b.m_x > 1.0F) {
        ++state.m_score1;
        resetBall(state, -1.0F);
        events |= HitEvent::ScoreLeft;
    } else if (b.m_x < -1.0F) {
        ++state.m_score2;
        resetBall(state, 1.0F);
        events |= HitEvent::ScoreRight;
    }

    return events;
}
