//
// Pong — minimal multiplayer with sound
//
// Usage:
//   ./pong host          → start as host (left paddle, binds :7755)
//   ./pong <host_ip>     → start as client (right paddle, connects to host)
//
// Network: UDP via QUdpSocket
//   Client → Host : InputPacket  (1 float: paddle direction)
//   Host   → Client: StatePacket  (ball + paddles + scores)
//
// Sound: engine AudioEngine abstraction (backed by miniaudio) — no .wav files needed.
//
// Engine features used:
//   Application + LayerStack  — game loop + layer dispatch
//   Layer (PongLayer)         — encapsulates all game logic
//   Scene + Entity            — ball, paddles, net, scorebars as ECS entities
//   Camera2D                  — NDC ortho projection
//   Renderer2D                — batched quad rendering
//   AudioEngine / AudioClip   — abstract sound backend
//

#include <application.h>
#include <audioclip.h>
#include <audioengine.h>
#include <camera2d.h>
#include <components.h>
#include <glfwwindow.h>
#include <layer.h>
#include <miniaudioengine.h>
#include <openglrenderer.h>
#include <renderer2d.h>
#include <scene.h>
#include <window.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QObject>
#include <QUdpSocket>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr quint16 kHostPort = 7755;
static constexpr int kSampleRate = 44100;

// NDC coordinates (-1..1 on both axes)
static constexpr float kPaddleW = 0.04F;
static constexpr float kPaddleH = 0.30F;
static constexpr float kPaddleSpeed = 1.5F;
static constexpr float kBallSize = 0.04F;
static constexpr float kBallSpeedInitial = 0.8F;
static constexpr float kBallSpeedMax = 2.0F;
static constexpr float kBallSpeedIncrease = 1.08F;

static constexpr float kLeftPaddleX = -0.92F;
static constexpr float kRightPaddleX = 0.88F;
static constexpr int kNetSegments = 10;
static constexpr float kMaxScore = 7.0F;

// ── Network packets ───────────────────────────────────────────────────────────

enum class PacketType : quint8 {
    Input = 1,
    State = 2
};

enum class Direction : qint8 {
    Down = -1,
    Idle = 0,
    Up = 1
};

// ── Game state ────────────────────────────────────────────────────────────────

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

// ── Packets ───────────────────────────────────────────────────────────────────

struct InputPacket {
    Direction m_direction = Direction::Idle;
};

static QByteArray serialize(const InputPacket &p)
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << static_cast<quint8>(PacketType::Input) << static_cast<qint8>(p.m_direction);
    return buf;
}

static QByteArray serialize(const GameState &s)
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << static_cast<quint8>(PacketType::State) << s.m_ball.m_x << s.m_ball.m_y << s.m_p1Y << s.m_p2Y << s.m_score1 << s.m_score2;
    return buf;
}

static bool deserialize(const QByteArray &buf, InputPacket &out)
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

static bool deserialize(const QByteArray &buf, GameState &out)
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

// ── Sine helper ───────────────────────────────────────────────────────────────

static std::vector<float> makeSine(float freq, float duration)
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

// ── PongLayer ─────────────────────────────────────────────────────────────────

class PongLayer : public engine::Layer, public QObject
{
public:
    enum class Role : quint8 {
        Host,
        Client
    };

    PongLayer(engine::Window &window, engine::AudioEngine &audio, Role role, const QString &hostIp)
        : engine::Layer(QStringLiteral("pong"))
        , m_window(window)
        , m_audio(audio)
        , m_role(role)
    {
        if (!hostIp.isEmpty()) {
            m_hostAddress = QHostAddress(hostIp);
        }
    }

    // Called by LayerStack when pushed onto Application.
    void onAttach() override
    {
        // Init Renderer2D — requires an active GL context (already set up by OpenGLRenderer).
        m_r2d.init();

        // Create audio clips — non-fatal if no device.
        m_hitClip = m_audio.createSound(makeSine(440.0F, 0.08F), kSampleRate);
        m_scoreClip = m_audio.createSound(makeSine(220.0F, 0.40F), kSampleRate);

        // Build the scene — one entity per visual object.
        m_ball = m_scene.createEntity("ball");
        m_ball.addComponent<engine::Transform>().scale = {kBallSize, kBallSize};
        m_ball.addComponent<engine::Sprite>().color = kWhite;

        m_p1 = m_scene.createEntity("p1");
        m_p1.addComponent<engine::Transform>().scale = {kPaddleW, kPaddleH};
        m_p1.addComponent<engine::Sprite>().color = kWhite;

        m_p2 = m_scene.createEntity("p2");
        m_p2.addComponent<engine::Transform>().scale = {kPaddleW, kPaddleH};
        m_p2.addComponent<engine::Sprite>().color = kWhite;

        for (int i = 0; i < kNetSegments; ++i) {
            auto seg = m_scene.createEntity("net");
            auto &tf = seg.addComponent<engine::Transform>();
            tf.position = {-0.01F, -0.95F + static_cast<float>(i) * 0.20F};
            tf.scale = {0.02F, 0.09F};
            seg.addComponent<engine::Sprite>().color = kDim;
            m_net[static_cast<std::size_t>(i)] = seg;
        }

        m_score1Bar = m_scene.createEntity("score1");
        m_score1Bar.addComponent<engine::Transform>().position = {-0.98F, 0.90F};
        m_score1Bar.addComponent<engine::Sprite>().color = kRed;

        m_score2Bar = m_scene.createEntity("score2");
        m_score2Bar.addComponent<engine::Transform>().position = {0.54F, 0.90F};
        m_score2Bar.addComponent<engine::Sprite>().color = kBlue;

        // Network socket
        m_socket = new QUdpSocket(this);
        if (m_role == Role::Host) {
            m_socket->bind(QHostAddress::AnyIPv4, kHostPort);
        } else {
            m_socket->bind(QHostAddress::AnyIPv4, 0);
        }
        QObject::connect(m_socket, &QUdpSocket::readyRead, this, &PongLayer::onReadyRead);
    }

    void onDetach() override
    {
        m_hitClip.reset();
        m_scoreClip.reset();
        delete m_socket;
        m_socket = nullptr;
    }

    void onUpdate(float dt) override
    {
        if (m_role == Role::Host) {
            hostUpdate(dt);
        } else {
            clientUpdate();
        }
        syncScene();
    }

    void onRender() override
    {
        m_scene.onRender(m_r2d, m_cam);
    }

private:
    // ── Colors ───────────────────────────────────────────────────────────────

    static constexpr glm::vec4 kWhite{1.0F, 1.0F, 1.0F, 1.0F};
    static constexpr glm::vec4 kDim{0.25F, 0.25F, 0.25F, 1.0F};
    static constexpr glm::vec4 kRed{1.0F, 0.25F, 0.25F, 1.0F};
    static constexpr glm::vec4 kBlue{0.25F, 0.50F, 1.0F, 1.0F};

    // ── Sound helpers ────────────────────────────────────────────────────────

    void playHit()
    {
        if (m_hitClip) {
            m_hitClip->play();
        }
    }

    void playScore()
    {
        if (m_scoreClip) {
            m_scoreClip->play();
        }
    }

    // ── Sync game state → scene components ──────────────────────────────────

    void syncScene()
    {
        // Ball
        m_ball.getComponent<engine::Transform>().position = {
            m_state.m_ball.m_x - kBallSize / 2.0F,
            m_state.m_ball.m_y - kBallSize / 2.0F,
        };

        // Paddles
        m_p1.getComponent<engine::Transform>().position = {kLeftPaddleX, m_state.m_p1Y - kPaddleH / 2.0F};
        m_p2.getComponent<engine::Transform>().position = {kRightPaddleX, m_state.m_p2Y - kPaddleH / 2.0F};

        // Score bars — scale width proportionally, hide if zero
        const float s1 = std::min(static_cast<float>(m_state.m_score1) / kMaxScore, 1.0F) * 0.44F;
        const float s2 = std::min(static_cast<float>(m_state.m_score2) / kMaxScore, 1.0F) * 0.44F;
        m_score1Bar.getComponent<engine::Transform>().scale = {s1 > 0.0F ? s1 : 0.0F, 0.05F};
        m_score2Bar.getComponent<engine::Transform>().scale = {s2 > 0.0F ? s2 : 0.0F, 0.05F};
    }

    // ── Host ─────────────────────────────────────────────────────────────────

    void hostUpdate(float dt)
    {
        m_state.m_p1Y = movePaddle(m_state.m_p1Y, localDir(), dt);
        m_state.m_p2Y = movePaddle(m_state.m_p2Y, m_remoteDir, dt);
        simulateBall(dt);
        sendState();
    }

    static float movePaddle(float y, Direction dir, float dt)
    {
        const float step = static_cast<float>(static_cast<qint8>(dir)) * kPaddleSpeed * dt;
        return std::clamp(y + step, -1.0F + kPaddleH / 2.0F, 1.0F - kPaddleH / 2.0F);
    }

    void simulateBall(float dt)
    {
        Ball &b = m_state.m_ball;
        b.m_x += b.m_vx * dt;
        b.m_y += b.m_vy * dt;

        // Wall bounce
        if (b.m_y + kBallSize / 2.0F >= 1.0F || b.m_y - kBallSize / 2.0F <= -1.0F) {
            b.m_vy = -b.m_vy;
            b.m_y = std::clamp(b.m_y, -1.0F + kBallSize / 2.0F, 1.0F - kBallSize / 2.0F);
            playHit();
        }

        // Paddle collisions
        const bool leftHit = b.m_vx < 0.0F && b.m_x - kBallSize / 2.0F <= kLeftPaddleX + kPaddleW && b.m_x + kBallSize / 2.0F >= kLeftPaddleX
            && std::abs(b.m_y - m_state.m_p1Y) < (kPaddleH + kBallSize) / 2.0F;

        const bool rightHit = b.m_vx > 0.0F && b.m_x + kBallSize / 2.0F >= kRightPaddleX && b.m_x - kBallSize / 2.0F <= kRightPaddleX + kPaddleW
            && std::abs(b.m_y - m_state.m_p2Y) < (kPaddleH + kBallSize) / 2.0F;

        if (leftHit) {
            b.m_vx = std::min(-b.m_vx * kBallSpeedIncrease, kBallSpeedMax);
            b.m_vy = ((b.m_y - m_state.m_p1Y) / (kPaddleH / 2.0F)) * std::abs(b.m_vx) * 0.75F;
            playHit();
        }
        if (rightHit) {
            b.m_vx = std::max(-b.m_vx * kBallSpeedIncrease, -kBallSpeedMax);
            b.m_vy = ((b.m_y - m_state.m_p2Y) / (kPaddleH / 2.0F)) * std::abs(b.m_vx) * 0.75F;
            playHit();
        }

        // Scoring
        if (b.m_x > 1.0F) {
            ++m_state.m_score1;
            playScore();
            m_state.m_ball = {
                .m_x = 0.0F,
                .m_y = 0.0F,
                .m_vx = -kBallSpeedInitial,
                .m_vy = kBallSpeedInitial * 0.5F,
            };
        } else if (b.m_x < -1.0F) {
            ++m_state.m_score2;
            playScore();
            m_state.m_ball = {
                .m_x = 0.0F,
                .m_y = 0.0F,
                .m_vx = kBallSpeedInitial,
                .m_vy = kBallSpeedInitial * 0.5F,
            };
        }
    }

    void sendState()
    {
        if (m_clientAddress.isNull()) {
            return;
        }
        m_socket->writeDatagram(serialize(m_state), m_clientAddress, m_clientPort);
    }

    // ── Client ───────────────────────────────────────────────────────────────

    void clientUpdate()
    {
        m_socket->writeDatagram(serialize(InputPacket{localDir()}), m_hostAddress, kHostPort);
    }

    // ── Shared ───────────────────────────────────────────────────────────────

    Direction localDir() const
    {
        if (m_window.isKeyPressed(engine::Key::W) || m_window.isKeyPressed(engine::Key::Up)) {
            return Direction::Up;
        }
        if (m_window.isKeyPressed(engine::Key::S) || m_window.isKeyPressed(engine::Key::Down)) {
            return Direction::Down;
        }
        return Direction::Idle;
    }

    void onReadyRead()
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

    // ── Members ──────────────────────────────────────────────────────────────

    engine::Window &m_window;
    engine::AudioEngine &m_audio;
    Role m_role;

    GameState m_state;
    engine::Scene m_scene;
    engine::Camera2D m_cam{-1.F, 1.F, -1.F, 1.F};
    engine::Renderer2D m_r2d;

    // Scene entities
    engine::Entity m_ball;
    engine::Entity m_p1;
    engine::Entity m_p2;
    std::array<engine::Entity, kNetSegments> m_net;
    engine::Entity m_score1Bar;
    engine::Entity m_score2Bar;

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

// ── PongGame ──────────────────────────────────────────────────────────────────

class PongGame : public engine::Application
{
public:
    bool onInit(PongLayer::Role role, const QString &hostIp)
    {
        auto layer = std::make_unique<PongLayer>(window(), audioEngine(), role, hostIp);
        pushLayer(std::move(layer));
        return true;
    }
};

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication qtApp(argc, argv);

    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2) {
        qCritical("Usage:  pong host  |  pong <host_ip>");
        return 1;
    }

    const bool isHost = (args.at(1) == u"host");
    const QString hostIp = isHost ? QString{} : args.at(1);

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, isHost ? "Pong — Host (W/S)" : "Pong — Client (W/S)")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();
    auto audio = std::make_unique<engine::MiniaudioEngine>();

    PongGame game;
    if (!game.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }
    if (!game.onInit(isHost ? PongLayer::Role::Host : PongLayer::Role::Client, hostIp)) {
        return 1;
    }

    return game.run();
}
