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
// Sound: miniaudio (header-only), procedural sine beeps — no .wav files needed.
//

// miniaudio implementation — must be defined exactly once.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <application.h>
#include <glfwwindow.h>
#include <openglrenderer.h>
#include <renderer2d.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QUdpSocket>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr quint16 kHostPort = 7755;

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

// ── Network packets ───────────────────────────────────────────────────────────
//
// QDataStream handles endianness (always big-endian on the wire) and alignment.
// No #pragma pack, no reinterpret_cast, no memcpy — just type-safe operators.
//
// A one-byte tag distinguishes packet types so both directions share one port.

enum class PacketType : quint8 {
    Input = 1,
    State = 2
};

// Direction is one of three states — qint8 is more expressive and cheaper
// on the wire than a float.
enum class Direction : qint8 {
    Down = -1,
    Idle = 0,
    Up = 1
};

// ── Game state ────────────────────────────────────────────────────────────────

// Single source of truth for both host simulation and client rendering.
// Ball velocity (m_vx, m_vy) is only meaningful on the host — the client
// leaves them at 0 since it doesn't simulate physics.
// Serialization selectively marshals only the fields that cross the wire.

struct Ball {
    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_vx = kBallSpeedInitial; // host only — not serialized
    float m_vy = kBallSpeedInitial * 0.6F; // host only — not serialized
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

// Only positions and scores cross the wire — velocity stays on the host.
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

// Deserialize into GameState — velocity fields are untouched (stay at default 0).
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

// ── Sound ─────────────────────────────────────────────────────────────────────

class Sound
{
public:
    Sound() = default;

    bool init()
    {
        if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS) {
            return false;
        }
        m_hitSamples = makeSine(440.0F, 0.08F);
        m_scoreSamples = makeSine(220.0F, 0.40F);
        setupSound(m_hitSound, m_hitBuf, m_hitSamples);
        setupSound(m_scoreSound, m_scoreBuf, m_scoreSamples);
        m_valid = true;
        return true;
    }

    ~Sound()
    {
        if (!m_valid) {
            return;
        }
        ma_sound_uninit(&m_hitSound);
        ma_sound_uninit(&m_scoreSound);
        ma_audio_buffer_uninit(&m_hitBuf);
        ma_audio_buffer_uninit(&m_scoreBuf);
        ma_engine_uninit(&m_engine);
    }

    Sound(const Sound &) = delete;
    Sound &operator=(const Sound &) = delete;

    void playHit()
    {
        replay(m_hitSound);
    }
    void playScore()
    {
        replay(m_scoreSound);
    }

private:
    static constexpr int kSampleRate = 44100;

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

    void setupSound(ma_sound &sound, ma_audio_buffer &buf, std::vector<float> &samples)
    {
        ma_audio_buffer_config cfg = ma_audio_buffer_config_init(ma_format_f32, 1, samples.size(), samples.data(), nullptr);
        cfg.sampleRate = kSampleRate;
        ma_audio_buffer_init(&cfg, &buf);
        ma_sound_init_from_data_source(&m_engine, &buf, 0, nullptr, &sound);
    }

    static void replay(ma_sound &sound)
    {
        ma_sound_seek_to_pcm_frame(&sound, 0);
        ma_sound_start(&sound);
    }

    ma_engine m_engine{};
    ma_audio_buffer m_hitBuf{};
    ma_audio_buffer m_scoreBuf{};
    ma_sound m_hitSound{};
    ma_sound m_scoreSound{};
    std::vector<float> m_hitSamples;
    std::vector<float> m_scoreSamples;
    bool m_valid = false;
};

// ── PongGame ──────────────────────────────────────────────────────────────────

class PongGame : public engine::Application
{
public:
    enum class Role : quint8 {
        Host,
        Client
    };

    bool onInit(Role role, const QString &hostIp)
    {
        m_role = role;

        if (!m_r2d.init()) {
            return false;
        }
        m_sound.init(); // non-fatal if audio device unavailable

        m_socket = new QUdpSocket(this);
        if (m_role == Role::Host) {
            m_socket->bind(QHostAddress::AnyIPv4, kHostPort);
        } else {
            m_socket->bind(QHostAddress::AnyIPv4, 0);
            m_hostAddress = QHostAddress(hostIp);
        }
        QObject::connect(m_socket, &QUdpSocket::readyRead, this, &PongGame::onReadyRead);
        return true;
    }

protected:
    void onUpdate(float dt) override
    {
        if (m_role == Role::Host) {
            hostUpdate(dt);
        } else {
            clientUpdate();
        }
    }

    void onRender() override
    {
        const glm::vec4 white{1.0F, 1.0F, 1.0F, 1.0F};
        const glm::vec4 dim{0.25F, 0.25F, 0.25F, 1.0F};
        const glm::vec4 red{1.0F, 0.25F, 0.25F, 1.0F};
        const glm::vec4 blue{0.25F, 0.50F, 1.0F, 1.0F};

        m_r2d.beginScene();

        // Net — 10 dashed segments
        for (int i = 0; i < 10; ++i) {
            m_r2d.drawQuad({-0.01F, -0.95F + static_cast<float>(i) * 0.20F}, {0.02F, 0.09F}, dim);
        }

        // Paddles
        m_r2d.drawQuad({kLeftPaddleX, m_state.m_p1Y - kPaddleH / 2.0F}, {kPaddleW, kPaddleH}, white);
        m_r2d.drawQuad({kRightPaddleX, m_state.m_p2Y - kPaddleH / 2.0F}, {kPaddleW, kPaddleH}, white);

        // Ball
        m_r2d.drawQuad({m_state.m_ball.m_x - kBallSize / 2.0F, m_state.m_ball.m_y - kBallSize / 2.0F}, {kBallSize, kBallSize}, white);

        // Score bars (proportional fill, max 7 points)
        static constexpr float kMaxScore = 7.0F;
        const float s1 = std::min(static_cast<float>(m_state.m_score1) / kMaxScore, 1.0F) * 0.44F;
        const float s2 = std::min(static_cast<float>(m_state.m_score2) / kMaxScore, 1.0F) * 0.44F;
        if (s1 > 0.0F) {
            m_r2d.drawQuad({-0.98F, 0.90F}, {s1, 0.05F}, red);
        }
        if (s2 > 0.0F) {
            m_r2d.drawQuad({0.54F, 0.90F}, {s2, 0.05F}, blue);
        }

        m_r2d.endScene();
    }

private:
    // ── Host ────────────────────────────────────────────────────────────────

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
            m_sound.playHit();
        }

        // Paddle collisions
        const bool leftHit = b.m_vx < 0.0F && b.m_x - kBallSize / 2.0F <= kLeftPaddleX + kPaddleW && b.m_x + kBallSize / 2.0F >= kLeftPaddleX
            && std::abs(b.m_y - m_state.m_p1Y) < (kPaddleH + kBallSize) / 2.0F;

        const bool rightHit = b.m_vx > 0.0F && b.m_x + kBallSize / 2.0F >= kRightPaddleX && b.m_x - kBallSize / 2.0F <= kRightPaddleX + kPaddleW
            && std::abs(b.m_y - m_state.m_p2Y) < (kPaddleH + kBallSize) / 2.0F;

        if (leftHit) {
            b.m_vx = std::min(-b.m_vx * kBallSpeedIncrease, kBallSpeedMax);
            b.m_vy = ((b.m_y - m_state.m_p1Y) / (kPaddleH / 2.0F)) * std::abs(b.m_vx) * 0.75F;
            m_sound.playHit();
        }
        if (rightHit) {
            b.m_vx = std::max(-b.m_vx * kBallSpeedIncrease, -kBallSpeedMax);
            b.m_vy = ((b.m_y - m_state.m_p2Y) / (kPaddleH / 2.0F)) * std::abs(b.m_vx) * 0.75F;
            m_sound.playHit();
        }

        // Scoring
        if (b.m_x > 1.0F) {
            ++m_state.m_score1;
            m_sound.playScore();
            m_state.m_ball = {
                .m_x = 0.0F,
                .m_y = 0.0F,
                .m_vx = -kBallSpeedInitial,
                .m_vy = kBallSpeedInitial * 0.5F,
            };
        } else if (b.m_x < -1.0F) {
            ++m_state.m_score2;
            m_sound.playScore();
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

    // ── Client ──────────────────────────────────────────────────────────────

    void clientUpdate()
    {
        m_socket->writeDatagram(serialize(InputPacket{localDir()}), m_hostAddress, kHostPort);
    }

    // ── Shared ──────────────────────────────────────────────────────────────

    Direction localDir() const
    {
        const auto &w = window();
        if (w.isKeyPressed(engine::Key::W) || w.isKeyPressed(engine::Key::Up)) {
            return Direction::Up;
        }
        if (w.isKeyPressed(engine::Key::S) || w.isKeyPressed(engine::Key::Down)) {
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

    Role m_role = Role::Host;
    GameState m_state;
    engine::Renderer2D m_r2d;
    Sound m_sound;
    QUdpSocket *m_socket = nullptr;

    // Host
    QHostAddress m_clientAddress;
    quint16 m_clientPort = 0;
    Direction m_remoteDir = Direction::Idle;

    // Client
    QHostAddress m_hostAddress;
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

    PongGame game;
    if (!game.init(std::move(window), std::move(renderer))) {
        return 1;
    }
    if (!game.onInit(isHost ? PongGame::Role::Host : PongGame::Role::Client, hostIp)) {
        return 1;
    }

    return game.run();
}
