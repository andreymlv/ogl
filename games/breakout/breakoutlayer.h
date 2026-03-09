#pragma once

#include "breakoutscene.h"
#include "constants.h"
#include "physics.h"

#include <audioclip.h>
#include <audioengine.h>
#include <camera2d.h>
#include <layer.h>
#include <renderer2d.h>
#include <texture.h>
#include <window.h>

#include <QObject>
#include <QSettings>

#include <memory>
#include <vector>

class BreakoutLayer : public engine::Layer, public QObject
{
public:
    BreakoutLayer(engine::Window &window, engine::AudioEngine &audio);

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender() override;

private:
    enum class Phase : int {
        Ready,
        Playing,
        GameOver,
        Victory,
    };

    std::shared_ptr<engine::Texture> loadTexture(const QString &name);
    void resetBall();
    void resetGame();
    void renderImGui();

    engine::Window &m_window;
    engine::AudioEngine &m_audio;

    engine::Renderer2D m_r2d;
    engine::Camera2D m_cam{0.F, kWorldW, 0.F, kWorldH};
    engine::Scene m_scene;

    BreakoutTextures m_textures;
    BreakoutEntities m_entities;

    // Audio clips
    std::unique_ptr<engine::AudioClip> m_hitClip;
    std::unique_ptr<engine::AudioClip> m_brickClip;
    std::unique_ptr<engine::AudioClip> m_loseClip;
    std::unique_ptr<engine::AudioClip> m_wallClip;

    // Game state
    Phase m_phase = Phase::Ready;
    float m_paddleX = kWorldW / 2.F - kPaddleW / 2.F;
    float m_paddleW = kPaddleW;
    bool m_paddleNarrow = false;
    BallState m_ball;
    std::vector<BrickState> m_bricks;
    float m_ballSpeed = kBallSpeedInitial;
    int m_score = 0;
    int m_lives = kStartLives;
    int m_hiScore = 0;
    bool m_ceilingHit = false; // has ball reached ceiling this life?

    // Persistent settings
    QSettings m_settings{QStringLiteral("Breakout"), QStringLiteral("Breakout")};

    // Input edge detection
    bool m_spaceWasPressed = false;

    // Cached gamepad slot
    int m_gamepadSlot = -1;
};
