#pragma once

#include <glm/glm.hpp>

#include <QtTypes>

// ── Gameplay constants ───────────────────────────────────────────────────────

inline constexpr quint16 kHostPort = 7755;
inline constexpr int kSampleRate = 44100;

// NDC coordinates (-1..1 on both axes)
inline constexpr float kPaddleW = 0.04F;
inline constexpr float kPaddleH = 0.30F;
inline constexpr float kPaddleSpeed = 1.5F;
inline constexpr float kBallSize = 0.04F;
inline constexpr float kBallSpeedInitial = 0.8F;
inline constexpr float kBallSpeedMax = 2.0F;
inline constexpr float kBallSpeedIncrease = 1.08F;

inline constexpr float kLeftPaddleX = -0.92F;
inline constexpr float kRightPaddleX = 0.88F;
inline constexpr int kNetSegments = 10;
inline constexpr float kMaxScore = 7.0F;

// ── Colors ───────────────────────────────────────────────────────────────────

inline constexpr glm::vec4 kWhite{1.0F, 1.0F, 1.0F, 1.0F};
inline constexpr glm::vec4 kDim{0.25F, 0.25F, 0.25F, 1.0F};
inline constexpr glm::vec4 kRed{1.0F, 0.25F, 0.25F, 1.0F};
inline constexpr glm::vec4 kBlue{0.25F, 0.50F, 1.0F, 1.0F};
