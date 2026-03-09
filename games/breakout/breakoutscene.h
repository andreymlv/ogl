#pragma once

#include "constants.h"
#include "physics.h"

#include <components.h>
#include <scene.h>
#include <texture.h>

#include <cstddef>
#include <memory>
#include <vector>

// ── Textures ────────────────────────────────────────────────────────────────

struct BreakoutTextures {
    std::shared_ptr<engine::Texture> background;
    std::shared_ptr<engine::Texture> frame;
    std::shared_ptr<engine::Texture> paddle;
    std::shared_ptr<engine::Texture> paddleFlash;
    std::shared_ptr<engine::Texture> ball;
    std::array<std::shared_ptr<engine::Texture>, 9> brickTex; // Brick1..Brick9
};

// ── Entity containers ───────────────────────────────────────────────────────

struct BreakoutEntities {
    engine::Entity background;
    engine::Entity frame;
    engine::Entity paddle;
    engine::Entity ball;
    std::vector<engine::Entity> bricks;
};

// ── Build scene ─────────────────────────────────────────────────────────────

inline BreakoutEntities buildBreakoutScene(engine::Scene &scene, const BreakoutTextures &tex, const std::vector<BrickState> &bricks)
{
    BreakoutEntities ents;

    // Background — zOrder 0
    ents.background = scene.createEntity("background");
    auto &bgTf = ents.background.addComponent<engine::Transform>();
    bgTf.position = {0.F, 0.F};
    bgTf.scale = {kWorldW, kWorldH};
    auto &bgSp = ents.background.addComponent<engine::Sprite>();
    bgSp.texture = tex.background;
    bgSp.zOrder = 0;

    // Frame overlay — zOrder 3 (on top of bricks but we render it as the border)
    ents.frame = scene.createEntity("frame");
    auto &frTf = ents.frame.addComponent<engine::Transform>();
    frTf.position = {0.F, 0.F};
    frTf.scale = {kWorldW, kWorldH};
    auto &frSp = ents.frame.addComponent<engine::Sprite>();
    frSp.texture = tex.frame;
    frSp.zOrder = 3;

    // Bricks — zOrder 1
    ents.bricks.reserve(bricks.size());
    for (std::size_t i = 0; i < bricks.size(); ++i) {
        const auto &bs = bricks[i];
        auto e = scene.createEntity("brick");
        auto &tf = e.addComponent<engine::Transform>();
        tf.position = {bs.x(), bs.y()};
        tf.scale = {kBrickW, kBrickH};
        auto &sp = e.addComponent<engine::Sprite>();
        // Use a brick texture that corresponds to the row color
        // Map row to brick texture index (1-indexed in filenames, 0-indexed in array)
        const auto texIdx = static_cast<std::size_t>(bs.row % 9);
        sp.texture = tex.brickTex[texIdx];
        sp.color = kRowColors[bs.row];
        sp.zOrder = 1;
        ents.bricks.push_back(e);
    }

    // Paddle — zOrder 2
    ents.paddle = scene.createEntity("paddle");
    auto &padTf = ents.paddle.addComponent<engine::Transform>();
    padTf.position = {kWorldW / 2.F - kPaddleW / 2.F, kPaddleY};
    padTf.scale = {kPaddleW, kPaddleH};
    auto &padSp = ents.paddle.addComponent<engine::Sprite>();
    padSp.texture = tex.paddle;
    padSp.zOrder = 2;

    // Ball — zOrder 2
    ents.ball = scene.createEntity("ball");
    auto &ballTf = ents.ball.addComponent<engine::Transform>();
    ballTf.position = {kWorldW / 2.F - kBallSize / 2.F, kPaddleY + kPaddleH + 2.F};
    ballTf.scale = {kBallSize, kBallSize};
    auto &ballSp = ents.ball.addComponent<engine::Sprite>();
    ballSp.texture = tex.ball;
    ballSp.zOrder = 2;

    return ents;
}

// ── Sync scene from game state ──────────────────────────────────────────────

inline void syncBreakoutScene(BreakoutEntities &ents,
                              const BreakoutTextures &tex,
                              float paddleX,
                              float paddleW,
                              bool paddleNarrow,
                              const BallState &ball,
                              const std::vector<BrickState> &bricks)
{
    // Paddle
    auto &padTf = ents.paddle.getComponent<engine::Transform>();
    padTf.position = {paddleX, kPaddleY};
    padTf.scale = {paddleW, kPaddleH};
    ents.paddle.getComponent<engine::Sprite>().texture = paddleNarrow ? tex.paddleFlash : tex.paddle;

    // Ball
    auto &ballTf = ents.ball.getComponent<engine::Transform>();
    ballTf.position = {ball.x, ball.y};

    // Bricks — hide destroyed ones
    for (std::size_t i = 0; i < bricks.size() && i < ents.bricks.size(); ++i) {
        auto &tf = ents.bricks[i].getComponent<engine::Transform>();
        if (bricks[i].alive) {
            tf.position = {bricks[i].x(), bricks[i].y()};
            tf.scale = {kBrickW, kBrickH};
        } else {
            tf.scale = {0.F, 0.F}; // hide
        }
    }
}
