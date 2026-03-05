#pragma once

#include "constants.h"

#include <components.h>
#include <scene.h>
#include <texture.h>

#include <array>
#include <memory>

// ── Entity containers ───────────────────────────────────────────────────────

struct PipePair {
    engine::Entity top;
    engine::Entity bottom;
    float gapCenterY = 0.F;
    float x = 0.F;
    bool scored = false;
};

struct FlappyEntities {
    engine::Entity background;
    engine::Entity bird;
    engine::Entity base1;
    engine::Entity base2;
    engine::Entity message;
    engine::Entity gameover;
    std::array<PipePair, kPipeCount> pipes;
    std::array<engine::Entity, kMaxDigits> digits;
};

// ── Build scene ─────────────────────────────────────────────────────────────

struct FlappyTextures {
    std::shared_ptr<engine::Texture> background;
    std::shared_ptr<engine::Texture> base;
    std::shared_ptr<engine::Texture> pipe;
    std::array<std::shared_ptr<engine::Texture>, 3> bird;
    std::shared_ptr<engine::Texture> message;
    std::shared_ptr<engine::Texture> gameover;
    std::array<std::shared_ptr<engine::Texture>, 10> digitTex;
};

inline FlappyEntities buildFlappyScene(engine::Scene &scene, const FlappyTextures &tex)
{
    FlappyEntities ents;

    // Background — zOrder 0
    ents.background = scene.createEntity("background");
    auto &bgTf = ents.background.addComponent<engine::Transform>();
    bgTf.position = {0.F, 0.F};
    bgTf.scale = {kWorldW, kWorldH};
    auto &bgSp = ents.background.addComponent<engine::Sprite>();
    bgSp.texture = tex.background;
    bgSp.zOrder = 0;

    // Pipes — zOrder 1
    for (int i = 0; i < kPipeCount; ++i) {
        auto &pp = ents.pipes[static_cast<std::size_t>(i)];

        // Bottom pipe (normal orientation)
        pp.bottom = scene.createEntity("pipe_bottom");
        auto &btf = pp.bottom.addComponent<engine::Transform>();
        btf.scale = {kPipeW, kPipeH};
        auto &bsp = pp.bottom.addComponent<engine::Sprite>();
        bsp.texture = tex.pipe;
        bsp.zOrder = 1;

        // Top pipe (flipped 180 degrees)
        pp.top = scene.createEntity("pipe_top");
        auto &ttf = pp.top.addComponent<engine::Transform>();
        ttf.scale = {kPipeW, kPipeH};
        ttf.rotation = 180.F;
        auto &tsp = pp.top.addComponent<engine::Sprite>();
        tsp.texture = tex.pipe;
        tsp.zOrder = 1;

        // Start off-screen
        pp.x = kWorldW + static_cast<float>(i) * kPipeSpacing;
        pp.gapCenterY = 250.F;
        pp.scored = false;
    }

    // Ground — zOrder 2 (two tiles for seamless scroll)
    ents.base1 = scene.createEntity("base1");
    auto &b1tf = ents.base1.addComponent<engine::Transform>();
    b1tf.position = {0.F, 0.F};
    b1tf.scale = {kBaseW, kBaseH};
    auto &b1sp = ents.base1.addComponent<engine::Sprite>();
    b1sp.texture = tex.base;
    b1sp.zOrder = 2;

    ents.base2 = scene.createEntity("base2");
    auto &b2tf = ents.base2.addComponent<engine::Transform>();
    b2tf.position = {kBaseW, 0.F};
    b2tf.scale = {kBaseW, kBaseH};
    auto &b2sp = ents.base2.addComponent<engine::Sprite>();
    b2sp.texture = tex.base;
    b2sp.zOrder = 2;

    // Bird — zOrder 3
    ents.bird = scene.createEntity("bird");
    auto &birdTf = ents.bird.addComponent<engine::Transform>();
    birdTf.position = {kBirdX, kBirdStartY};
    birdTf.scale = {kBirdW, kBirdH};
    auto &birdSp = ents.bird.addComponent<engine::Sprite>();
    birdSp.texture = tex.bird[0];
    birdSp.zOrder = 3;

    // Message overlay — zOrder 4
    ents.message = scene.createEntity("message");
    auto &msgTf = ents.message.addComponent<engine::Transform>();
    msgTf.position = {52.F, 180.F};
    msgTf.scale = {184.F, 267.F};
    auto &msgSp = ents.message.addComponent<engine::Sprite>();
    msgSp.texture = tex.message;
    msgSp.zOrder = 4;

    // Game over overlay — zOrder 4, hidden initially
    ents.gameover = scene.createEntity("gameover");
    auto &goTf = ents.gameover.addComponent<engine::Transform>();
    goTf.position = {48.F, 340.F};
    goTf.scale = {192.F, 42.F};
    auto &goSp = ents.gameover.addComponent<engine::Sprite>();
    goSp.texture = tex.gameover;
    goSp.zOrder = 4;
    goTf.scale = {0.F, 0.F}; // hidden

    // Score digits — zOrder 4
    for (int i = 0; i < kMaxDigits; ++i) {
        ents.digits[static_cast<std::size_t>(i)] = scene.createEntity("digit");
        auto &dtf = ents.digits[static_cast<std::size_t>(i)].addComponent<engine::Transform>();
        dtf.position = {0.F, kDigitY};
        dtf.scale = {0.F, 0.F}; // hidden initially
        auto &dsp = ents.digits[static_cast<std::size_t>(i)].addComponent<engine::Sprite>();
        dsp.texture = tex.digitTex[0];
        dsp.zOrder = 4;
    }

    return ents;
}

// ── Sync scene from game state ──────────────────────────────────────────────

enum class GamePhase {
    Ready,
    Playing,
    GameOver,
};

inline void syncFlappyScene(FlappyEntities &ents,
                            const FlappyTextures &tex,
                            float birdY,
                            float birdRotation,
                            int birdFrame,
                            float baseOffset,
                            GamePhase phase,
                            int score)
{
    // Bird
    auto &birdTf = ents.bird.getComponent<engine::Transform>();
    birdTf.position = {kBirdX, birdY};
    birdTf.rotation = birdRotation;
    ents.bird.getComponent<engine::Sprite>().texture = tex.bird[static_cast<std::size_t>(birdFrame)];

    // Ground scroll
    ents.base1.getComponent<engine::Transform>().position.x = -baseOffset;
    ents.base2.getComponent<engine::Transform>().position.x = kBaseW - baseOffset;

    // Pipes
    for (int i = 0; i < kPipeCount; ++i) {
        auto &pp = ents.pipes[static_cast<std::size_t>(i)];

        // Bottom pipe: top of pipe at gapCenterY - gap/2
        const float bottomPipeTop = pp.gapCenterY - kPipeGap / 2.F;
        auto &btf = pp.bottom.getComponent<engine::Transform>();
        btf.position = {pp.x, bottomPipeTop - kPipeH};

        // Top pipe (rotated 180): its "bottom" visually is at gapCenterY + gap/2
        const float topPipeBottom = pp.gapCenterY + kPipeGap / 2.F;
        auto &ttf = pp.top.getComponent<engine::Transform>();
        ttf.position = {pp.x, topPipeBottom};
    }

    // Message visibility
    auto &msgTf = ents.message.getComponent<engine::Transform>();
    msgTf.scale = (phase == GamePhase::Ready) ? glm::vec2{184.F, 267.F} : glm::vec2{0.F, 0.F};

    // Game over visibility
    auto &goTf = ents.gameover.getComponent<engine::Transform>();
    goTf.scale = (phase == GamePhase::GameOver) ? glm::vec2{192.F, 42.F} : glm::vec2{0.F, 0.F};

    // Score digits
    const int displayScore = (phase == GamePhase::Ready) ? 0 : score;
    const auto scoreStr = std::to_string(displayScore);
    const auto numDigits = static_cast<int>(scoreStr.size());
    const float totalW = static_cast<float>(numDigits) * kDigitW;
    const float startX = (kWorldW - totalW) / 2.F;

    for (int i = 0; i < kMaxDigits; ++i) {
        auto &dtf = ents.digits[static_cast<std::size_t>(i)].getComponent<engine::Transform>();
        auto &dsp = ents.digits[static_cast<std::size_t>(i)].getComponent<engine::Sprite>();
        if (i < numDigits) {
            const int digit = scoreStr[static_cast<std::size_t>(i)] - '0';
            dtf.position = {startX + static_cast<float>(i) * kDigitW, kDigitY};
            dtf.scale = {kDigitW, kDigitH};
            dsp.texture = tex.digitTex[static_cast<std::size_t>(digit)];
        } else {
            dtf.scale = {0.F, 0.F};
        }
    }
}
