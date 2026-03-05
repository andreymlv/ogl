#pragma once

#include "constants.h"
#include "flappyscene.h"
#include "physics.h"

#include <audioclip.h>
#include <audioengine.h>
#include <camera2d.h>
#include <layer.h>
#include <renderer2d.h>
#include <texture.h>
#include <window.h>

#include <QObject>

#include <array>
#include <memory>
#include <random>

class FlappyLayer : public engine::Layer, public QObject
{
public:
    FlappyLayer(engine::Window &window, engine::AudioEngine &audio);

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender() override;

private:
    std::shared_ptr<engine::Texture> loadTexture(const QString &name);
    std::unique_ptr<engine::AudioClip> loadAudio(const QString &name);

    engine::Window &m_window;
    engine::AudioEngine &m_audio;

    engine::Renderer2D m_r2d;
    engine::Camera2D m_cam{0.F, kWorldW, 0.F, kWorldH};
    engine::Scene m_scene;

    FlappyTextures m_textures;
    FlappyEntities m_entities;

    // Audio
    std::unique_ptr<engine::AudioClip> m_wingClip;
    std::unique_ptr<engine::AudioClip> m_pointClip;
    std::unique_ptr<engine::AudioClip> m_hitClip;
    std::unique_ptr<engine::AudioClip> m_dieClip;
    std::unique_ptr<engine::AudioClip> m_swooshClip;

    // Game state
    GamePhase m_phase = GamePhase::Ready;
    float m_birdY = kBirdStartY;
    float m_birdVy = 0.F;
    int m_score = 0;

    // Bird animation
    int m_birdFrame = 0;
    float m_animTimer = 0.F;

    // Bob animation (ready screen)
    float m_bobTimer = 0.F;

    // Ground scroll
    float m_baseOffset = 0.F;

    // Pipe state
    std::array<PipeState, kPipeCount> m_pipeState{};

    // RNG
    std::mt19937 m_rng{std::random_device{}()};

    // Input edge detection
    bool m_spaceWasPressed = false;
    bool m_aWasPressed = false;

    // AI autoplay
    bool m_aiEnabled = false;

    // Game over delay before restart
    float m_gameOverTimer = 0.F;
};
