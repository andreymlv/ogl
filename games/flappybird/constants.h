#pragma once

// ── World ───────────────────────────────────────────────────────────────────

inline constexpr float kWorldW = 288.F;
inline constexpr float kWorldH = 512.F;

// ── Bird ────────────────────────────────────────────────────────────────────

inline constexpr float kBirdW = 34.F;
inline constexpr float kBirdH = 24.F;
inline constexpr float kBirdX = 60.F;
inline constexpr float kBirdStartY = 256.F;
inline constexpr float kGravity = -800.F;
inline constexpr float kFlapImpulse = 280.F;
inline constexpr float kMaxFallSpeed = -400.F;
inline constexpr float kAnimInterval = 0.15F;

// ── Pipes ───────────────────────────────────────────────────────────────────

inline constexpr float kPipeW = 52.F;
inline constexpr float kPipeH = 320.F;
inline constexpr float kPipeGap = 100.F;
inline constexpr float kPipeSpeed = 120.F;
inline constexpr float kPipeSpacing = 180.F;
inline constexpr float kPipeMinGapY = 180.F;
inline constexpr float kPipeMaxGapY = 350.F;
inline constexpr int kPipeCount = 3;

// ── Ground ──────────────────────────────────────────────────────────────────

inline constexpr float kBaseH = 112.F;
inline constexpr float kBaseW = 336.F;
inline constexpr float kFloorY = kBaseH;

// ── Score digits ────────────────────────────────────────────────────────────

inline constexpr float kDigitW = 24.F;
inline constexpr float kDigitH = 36.F;
inline constexpr float kDigitY = 450.F;
inline constexpr int kMaxDigits = 3;

