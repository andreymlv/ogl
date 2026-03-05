#pragma once

#include "pongscene.h"

#include <audioclip.h>
#include <audioengine.h>
#include <camera2d.h>
#include <layer.h>
#include <renderer2d.h>
#include <window.h>

#include <QHostAddress>
#include <QObject>
#include <QString>

#include <memory>

class QUdpSocket;

class PongLayer : public engine::Layer, public QObject
{
public:
    enum class Role : quint8 {
        Host,
        Client
    };

    PongLayer(engine::Window &window, engine::AudioEngine &audio, Role role, const QString &hostIp);

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender() override;

private:
    void playHit();
    void playScore();

    // Host
    void hostUpdate(float dt);
    void sendState();

    // Client
    void clientUpdate();

    // Shared
    Direction localDir() const;
    void onReadyRead();

    // ── Members ──────────────────────────────────────────────────────────────

    engine::Window &m_window;
    engine::AudioEngine &m_audio;
    Role m_role;

    GameState m_state;
    engine::Scene m_scene;
    engine::Camera2D m_cam{-1.F, 1.F, -1.F, 1.F};
    engine::Renderer2D m_r2d;

    PongEntities m_entities;

    // Audio clips
    std::unique_ptr<engine::AudioClip> m_hitClip;
    std::unique_ptr<engine::AudioClip> m_scoreClip;

    QUdpSocket *m_socket = nullptr;

    // Host
    QHostAddress m_clientAddress;
    quint16 m_clientPort = 0;
    Direction m_remoteDir = Direction::Idle;

    // Client
    QHostAddress m_hostAddress;
};
