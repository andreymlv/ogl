#pragma once

#include "constants.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

#include <cmath>
#include <numbers>
#include <vector>

// ── Network packets ──────────────────────────────────────────────────────────

enum class PacketType : quint8 {
    Input = 1,
    State = 2
};

enum class Direction : qint8 {
    Down = -1,
    Idle = 0,
    Up = 1
};

// ── Game state ───────────────────────────────────────────────────────────────

struct Ball {
    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_vx = kBallSpeedInitial; // host only
    float m_vy = kBallSpeedInitial * 0.6F; // host only
};

struct GameState {
    Ball m_ball;
    float m_p1Y = 0.0F;
    float m_p2Y = 0.0F;
    qint32 m_score1 = 0;
    qint32 m_score2 = 0;
};

struct InputPacket {
    Direction m_direction = Direction::Idle;
};

// ── Serialization ────────────────────────────────────────────────────────────

inline QByteArray serialize(const InputPacket &p)
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << static_cast<quint8>(PacketType::Input) << static_cast<qint8>(p.m_direction);
    return buf;
}

inline QByteArray serialize(const GameState &s)
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << static_cast<quint8>(PacketType::State) << s.m_ball.m_x << s.m_ball.m_y << s.m_p1Y << s.m_p2Y << s.m_score1 << s.m_score2;
    return buf;
}

inline bool deserialize(const QByteArray &buf, InputPacket &out)
{
    QDataStream ds(buf);
    quint8 tag = 0;
    ds >> tag;
    if (ds.status() != QDataStream::Ok || tag != static_cast<quint8>(PacketType::Input)) {
        return false;
    }
    qint8 dir = 0;
    ds >> dir;
    out.m_direction = static_cast<Direction>(dir);
    return ds.status() == QDataStream::Ok;
}

inline bool deserialize(const QByteArray &buf, GameState &out)
{
    QDataStream ds(buf);
    quint8 tag = 0;
    ds >> tag;
    if (ds.status() != QDataStream::Ok || tag != static_cast<quint8>(PacketType::State)) {
        return false;
    }
    ds >> out.m_ball.m_x >> out.m_ball.m_y >> out.m_p1Y >> out.m_p2Y >> out.m_score1 >> out.m_score2;
    return ds.status() == QDataStream::Ok;
}

// ── Sine helper ──────────────────────────────────────────────────────────────

inline std::vector<float> makeSine(float freq, float duration)
{
    const int n = static_cast<int>(duration * kSampleRate);
    std::vector<float> buf(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        const float env = 1.0F - std::clamp((t / duration - 0.8F) / 0.2F, 0.0F, 1.0F);
        buf[static_cast<std::size_t>(i)] = 0.4F * env * std::sin(2.0F * std::numbers::pi_v<float> * freq * t);
    }
    return buf;
}
