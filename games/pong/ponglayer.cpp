#include "ponglayer.h"
#include "physics.h"

#include <QNetworkDatagram>
#include <QUdpSocket>

PongLayer::PongLayer(engine::Window &window, engine::AudioEngine &audio, Role role, const QString &hostIp)
    : engine::Layer(QStringLiteral("pong"))
    , m_window(window)
    , m_audio(audio)
    , m_role(role)
{
    if (!hostIp.isEmpty()) {
        m_hostAddress = QHostAddress(hostIp);
    }
}

void PongLayer::onAttach()
{
    // Init Renderer2D — requires an active GL context (already set up by OpenGLRenderer).
    m_r2d.init();

    // Create audio clips — non-fatal if no device.
    m_hitClip = m_audio.createSound(makeSine(440.0F, 0.08F), kSampleRate);
    m_scoreClip = m_audio.createSound(makeSine(220.0F, 0.40F), kSampleRate);

    // Build the scene — one entity per visual object.
    m_entities = buildPongScene(m_scene);

    // Network socket
    m_socket = new QUdpSocket(this);
    if (m_role == Role::Host) {
        m_socket->bind(QHostAddress::AnyIPv4, kHostPort);
    } else {
        m_socket->bind(QHostAddress::AnyIPv4, 0);
    }
    QObject::connect(m_socket, &QUdpSocket::readyRead, this, &PongLayer::onReadyRead);
}

void PongLayer::onDetach()
{
    m_hitClip.reset();
    m_scoreClip.reset();
    delete m_socket;
    m_socket = nullptr;
}

void PongLayer::onUpdate(float dt)
{
    if (m_role == Role::Host) {
        hostUpdate(dt);
    } else {
        clientUpdate();
    }
    syncPongScene(m_entities, m_state);
}

void PongLayer::onRender()
{
    m_scene.onRender(m_r2d, m_cam);
}

// ── Sound helpers ────────────────────────────────────────────────────────────

void PongLayer::playHit()
{
    if (m_hitClip) {
        m_hitClip->play();
    }
}

void PongLayer::playScore()
{
    if (m_scoreClip) {
        m_scoreClip->play();
    }
}

// ── Host ─────────────────────────────────────────────────────────────────────

void PongLayer::hostUpdate(float dt)
{
    m_state.m_p1Y = movePaddle(m_state.m_p1Y, localDir(), dt);
    m_state.m_p2Y = movePaddle(m_state.m_p2Y, m_remoteDir, dt);
    const auto events = simulateBall(m_state, dt);
    if (hasFlag(events, HitEvent::Wall) || hasFlag(events, HitEvent::Paddle)) {
        playHit();
    }
    if (hasFlag(events, HitEvent::ScoreLeft) || hasFlag(events, HitEvent::ScoreRight)) {
        playScore();
    }
    sendState();
}

void PongLayer::sendState()
{
    if (m_clientAddress.isNull()) {
        return;
    }
    m_socket->writeDatagram(serialize(m_state), m_clientAddress, m_clientPort);
}

// ── Client ───────────────────────────────────────────────────────────────────

void PongLayer::clientUpdate()
{
    m_socket->writeDatagram(serialize(InputPacket{localDir()}), m_hostAddress, kHostPort);
}

// ── Shared ───────────────────────────────────────────────────────────────────

Direction PongLayer::localDir() const
{
    if (m_window.isKeyPressed(engine::Key::W) || m_window.isKeyPressed(engine::Key::Up)) {
        return Direction::Up;
    }
    if (m_window.isKeyPressed(engine::Key::S) || m_window.isKeyPressed(engine::Key::Down)) {
        return Direction::Down;
    }
    return Direction::Idle;
}

void PongLayer::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram dg = m_socket->receiveDatagram();

        if (m_role == Role::Host) {
            m_clientAddress = dg.senderAddress();
            m_clientPort = static_cast<quint16>(dg.senderPort());
            InputPacket pkt;
            if (deserialize(dg.data(), pkt)) {
                m_remoteDir = pkt.m_direction;
            }
        } else {
            if (deserialize(dg.data(), m_state)) {
                m_state.m_ball.m_vx = 0.0F;
                m_state.m_ball.m_vy = 0.0F;
            }
        }
    }
}
