#pragma once

#include <glm/glm.hpp>

// ── World ───────────────────────────────────────────────────────────────────

inline constexpr float kWorldW = 800.F;
inline constexpr float kWorldH = 600.F;

// ── Paddle ──────────────────────────────────────────────────────────────────

inline constexpr float kPaddleW = 104.F;
inline constexpr float kPaddleNarrowW = 64.F; // narrower after ball hits ceiling
inline constexpr float kPaddleH = 24.F;
inline constexpr float kPaddleY = 30.F;
inline constexpr float kPaddleSpeed = 500.F;

// ── Ball ────────────────────────────────────────────────────────────────────

inline constexpr float kBallSize = 16.F;
inline constexpr float kBallSpeedInitial = 300.F;
inline constexpr float kBallSpeedMax = 700.F;
inline constexpr float kBallSpeedIncrease = 8.F; // added per brick broken

// ── Bricks ──────────────────────────────────────────────────────────────────

inline constexpr int kBrickCols = 14;
inline constexpr int kBrickRows = 8;
inline constexpr float kBrickW = 50.F;
inline constexpr float kBrickH = 20.F;
inline constexpr float kBrickPadding = 4.F;
inline constexpr float kBrickAreaTop = kWorldH - 40.F; // top of the brick area
inline constexpr float kBrickAreaLeft = (kWorldW - static_cast<float>(kBrickCols) * (kBrickW + kBrickPadding) + kBrickPadding) / 2.F;

// ── Frame / walls ───────────────────────────────────────────────────────────

inline constexpr float kWallThickness = 16.F;

// ── Gameplay ────────────────────────────────────────────────────────────────

inline constexpr int kStartLives = 3;
inline constexpr int kMaxLives = 5;

// ── Score values per row (bottom row = row 0, top row = row 7) ──────────────

inline constexpr int kRowScore[] = {1, 1, 2, 2, 3, 3, 5, 5};

// ── Row colors (classic Atari Breakout palette, bottom to top) ──────────────

inline constexpr glm::vec4 kRowColors[] = {
    {1.0F, 0.85F, 0.0F, 1.0F}, // row 0: yellow
    {1.0F, 0.85F, 0.0F, 1.0F}, // row 1: yellow
    {0.0F, 0.75F, 0.0F, 1.0F}, // row 2: green
    {0.0F, 0.75F, 0.0F, 1.0F}, // row 3: green
    {1.0F, 0.55F, 0.0F, 1.0F}, // row 4: orange
    {1.0F, 0.55F, 0.0F, 1.0F}, // row 5: orange
    {1.0F, 0.2F, 0.2F, 1.0F}, // row 6: red
    {1.0F, 0.2F, 0.2F, 1.0F}, // row 7: red
};

// ── Audio ───────────────────────────────────────────────────────────────────

inline constexpr int kSampleRate = 44100;
