#pragma once

#include "network.h"

#include <cmath>

struct AiState {
    float m_reactionTimer = 0.0F;
    float m_targetY = 0.0F;
    float m_offset = 0.0F;
};

inline constexpr float kAiReactionInterval = 0.3F;
inline constexpr float kAiDeadZone = 0.02F;
inline constexpr float kAiOffsetRange = 0.03F; // ±10% of paddle height

inline float predictBallY(const Ball &ball, float targetX)
{
    if (ball.m_vx == 0.0F) {
        return 0.0F;
    }
    const float time = (targetX - ball.m_x) / ball.m_vx;
    if (time < 0.0F) {
        return 0.0F; // ball moving away
    }
    float y = ball.m_y + (ball.m_vy * time);
    // Fold into [-1, 1] using triangle wave (period 4)
    y = std::fmod(y + 1.0F, 4.0F);
    if (y < 0.0F) {
        y += 4.0F;
    }
    if (y > 2.0F) {
        y = 4.0F - y;
    }
    return y - 1.0F;
}

inline Direction aiDirection(const Ball &ball, float paddleY, float paddleX, float dt, AiState &ai)
{
    // Determine if ball is coming toward this paddle
    const bool approaching = (paddleX < 0.0F) ? (ball.m_vx < 0.0F) : (ball.m_vx > 0.0F);

    ai.m_reactionTimer -= dt;
    if (ai.m_reactionTimer <= 0.0F) {
        ai.m_reactionTimer = kAiReactionInterval;
        if (approaching) {
            ai.m_targetY = predictBallY(ball, paddleX);
            // Simple deterministic offset based on ball position (pseudo-random)
            ai.m_offset = kAiOffsetRange * std::sin((ball.m_x * 17.0F) + (ball.m_y * 31.0F));
        } else {
            ai.m_targetY = 0.0F; // drift to center
            ai.m_offset = 0.0F;
        }
    }

    const float target = ai.m_targetY + ai.m_offset;
    const float diff = target - paddleY;
    if (std::abs(diff) < kAiDeadZone) {
        return Direction::Idle;
    }
    return (diff > 0.0F) ? Direction::Up : Direction::Down;
}
