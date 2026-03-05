#pragma once

#include "constants.h"
#include "network.h"

#include <components.h>
#include <scene.h>

#include <algorithm>
#include <array>
#include <utility>

struct PongEntities {
    engine::Entity ball;
    engine::Entity p1;
    engine::Entity p2;
    std::array<engine::Entity, kNetSegments> net;
    engine::Entity score1Bar;
    engine::Entity score2Bar;
};

inline PongEntities buildPongScene(engine::Scene &scene)
{
    PongEntities ents;

    ents.ball = scene.createEntity("ball");
    ents.ball.addComponent<engine::Transform>().scale = {kBallSize, kBallSize};
    ents.ball.addComponent<engine::Sprite>().color = kWhite;

    ents.p1 = scene.createEntity("p1");
    ents.p1.addComponent<engine::Transform>().scale = {kPaddleW, kPaddleH};
    ents.p1.addComponent<engine::Sprite>().color = kWhite;

    ents.p2 = scene.createEntity("p2");
    ents.p2.addComponent<engine::Transform>().scale = {kPaddleW, kPaddleH};
    ents.p2.addComponent<engine::Sprite>().color = kWhite;

    for (int i = 0; i < kNetSegments; ++i) {
        auto seg = scene.createEntity("net");
        auto &tf = seg.addComponent<engine::Transform>();
        tf.position = {-0.01F, -0.95F + static_cast<float>(i) * 0.20F};
        tf.scale = {0.02F, 0.09F};
        seg.addComponent<engine::Sprite>().color = kDim;
        ents.net[static_cast<std::size_t>(i)] = seg;
    }

    ents.score1Bar = scene.createEntity("score1");
    ents.score1Bar.addComponent<engine::Transform>().position = {-0.98F, 0.90F};
    ents.score1Bar.addComponent<engine::Sprite>().color = kRed;

    ents.score2Bar = scene.createEntity("score2");
    ents.score2Bar.addComponent<engine::Transform>().position = {0.54F, 0.90F};
    ents.score2Bar.addComponent<engine::Sprite>().color = kBlue;

    return ents;
}

inline void flipPerspective(GameState &state)
{
    state.m_ball.m_x = -state.m_ball.m_x;
    std::swap(state.m_p1Y, state.m_p2Y);
    std::swap(state.m_score1, state.m_score2);
}

inline void syncPongScene(PongEntities &ents, const GameState &state)
{
    // Ball
    ents.ball.getComponent<engine::Transform>().position = {
        state.m_ball.m_x - kBallSize / 2.0F,
        state.m_ball.m_y - kBallSize / 2.0F,
    };

    // Paddles
    ents.p1.getComponent<engine::Transform>().position = {kLeftPaddleX, state.m_p1Y - kPaddleH / 2.0F};
    ents.p2.getComponent<engine::Transform>().position = {kRightPaddleX, state.m_p2Y - kPaddleH / 2.0F};

    // Score bars — scale width proportionally, hide if zero
    const float s1 = std::min(static_cast<float>(state.m_score1) / kMaxScore, 1.0F) * 0.44F;
    const float s2 = std::min(static_cast<float>(state.m_score2) / kMaxScore, 1.0F) * 0.44F;
    ents.score1Bar.getComponent<engine::Transform>().scale = {s1 > 0.0F ? s1 : 0.0F, 0.05F};
    ents.score2Bar.getComponent<engine::Transform>().scale = {s2 > 0.0F ? s2 : 0.0F, 0.05F};
}
