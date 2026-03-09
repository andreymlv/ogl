#include "ponglayer.h"
#include "physics.h"

#include <imgui.h>

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

    // Network socket (not needed for solo)
    if (m_role != Role::Solo) {
        m_socket = new QUdpSocket(this);
        if (m_role == Role::Host) {
            m_socket->bind(QHostAddress::AnyIPv4, kHostPort);
        } else {
            m_socket->bind(QHostAddress::AnyIPv4, 0);
        }
        QObject::connect(m_socket, &QUdpSocket::readyRead, this, &PongLayer::onReadyRead);
    }
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
    // Discover the first connected gamepad (slot may change at runtime)
    m_gamepadSlot = m_window.firstGamepadSlot();

    // Space / gamepad A / Start edge detection (press, not hold)
    bool spaceDown = m_window.isKeyPressed(engine::Key::Space);
    if (m_gamepadSlot >= 0) {
        spaceDown = spaceDown || m_window.isGamepadButtonPressed(engine::GamepadButton::A, m_gamepadSlot)
            || m_window.isGamepadButtonPressed(engine::GamepadButton::Start, m_gamepadSlot);
    }
    const bool spacePressed = spaceDown && !m_spaceWasPressed;
    m_spaceWasPressed = spaceDown;

    // Phase transitions
    if (m_phase == Phase::Ready && spacePressed) {
        m_phase = Phase::Playing;
        resetGame();
    } else if (m_phase == Phase::GameOver && spacePressed) {
        m_phase = Phase::Ready;
    }

    // Only simulate when playing
    if (m_phase == Phase::Playing) {
        if (m_role == Role::Solo) {
            soloUpdate(dt);
        } else if (m_role == Role::Host) {
            hostUpdate(dt);
        } else {
            clientUpdate();
        }

        // Check win condition (solo / host only — client follows host state)
        if (m_role != Role::Client) {
            if (m_state.m_score1 >= static_cast<qint32>(kMaxScore) || m_state.m_score2 >= static_cast<qint32>(kMaxScore)) {
                m_phase = Phase::GameOver;
            }
        }
    } else if (m_role == Role::Client) {
        // Client still sends input and receives state even when not "playing" locally
        clientUpdate();
    }

    GameState renderState = m_state;
    if (m_role == Role::Client) {
        flipPerspective(renderState);
    }
    syncPongScene(m_entities, renderState);
}

void PongLayer::onRender()
{
    m_scene.onRender(m_r2d, m_cam);
    renderImGui();
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

void PongLayer::resetGame()
{
    m_state = GameState{};
    resetBall(m_state, 1.0F);
    m_aiP1 = AiState{};
    m_aiP2 = AiState{};
    m_prevState = GameState{};
}

// ── ImGui overlay ────────────────────────────────────────────────────────────

void PongLayer::renderImGui()
{
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 center{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F};

    // Always show score
    {
        ImGui::SetNextWindowPos({center.x, vp->WorkPos.y + 10.0F}, ImGuiCond_Always, {0.5F, 0.0F});
        ImGui::SetNextWindowBgAlpha(0.0F);
        ImGui::Begin("Score", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);

        GameState displayState = m_state;
        if (m_role == Role::Client) {
            flipPerspective(displayState);
        }
        ImGui::Text("%d  :  %d", displayState.m_score1, displayState.m_score2);
        ImGui::End();
    }

    // Phase-specific overlay
    if (m_phase == Phase::Ready) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.6F);
        ImGui::Begin("Ready", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("PONG");
        ImGui::Separator();
        ImGui::Text("W/S or Up/Down to move");
        if (m_gamepadSlot >= 0) {
            ImGui::Text("D-pad / Left Stick to move");
            ImGui::Text("Press A or START to begin");
        } else {
            ImGui::Text("Press SPACE to start");
        }
        ImGui::End();
    } else if (m_phase == Phase::GameOver) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.6F);
        ImGui::Begin("GameOver",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);

        GameState displayState = m_state;
        if (m_role == Role::Client) {
            flipPerspective(displayState);
        }
        if (displayState.m_score1 >= static_cast<qint32>(kMaxScore)) {
            ImGui::Text("PLAYER 1 WINS!");
        } else {
            ImGui::Text("PLAYER 2 WINS!");
        }
        ImGui::Text("Score: %d - %d", displayState.m_score1, displayState.m_score2);
        ImGui::Separator();
        if (m_gamepadSlot >= 0) {
            ImGui::Text("Press A or START to restart");
        } else {
            ImGui::Text("Press SPACE to restart");
        }
        ImGui::End();
    }
}

// ── Solo ─────────────────────────────────────────────────────────────────────

void PongLayer::soloUpdate(float dt)
{
    m_state.m_p1Y = movePaddle(m_state.m_p1Y, localDir(), dt);
    m_state.m_p2Y = movePaddle(m_state.m_p2Y, aiDirection(m_state.m_ball, m_state.m_p2Y, kRightPaddleX, dt, m_aiP2), dt);
    const auto events = simulateBall(m_state, dt);
    if (hasFlag(events, HitEvent::Wall) || hasFlag(events, HitEvent::Paddle)) {
        playHit();
    }
    if (hasFlag(events, HitEvent::ScoreLeft) || hasFlag(events, HitEvent::ScoreRight)) {
        playScore();
    }
}

// ── Host ─────────────────────────────────────────────────────────────────────

void PongLayer::hostUpdate(float dt)
{
    if (m_clientConnected) {
        m_state.m_p1Y = movePaddle(m_state.m_p1Y, localDir(), dt);
        m_state.m_p2Y = movePaddle(m_state.m_p2Y, m_remoteDir, dt);
    } else {
        // Demo mode: both paddles AI-controlled
        m_state.m_p1Y = movePaddle(m_state.m_p1Y, aiDirection(m_state.m_ball, m_state.m_p1Y, kLeftPaddleX, dt, m_aiP1), dt);
        m_state.m_p2Y = movePaddle(m_state.m_p2Y, aiDirection(m_state.m_ball, m_state.m_p2Y, kRightPaddleX, dt, m_aiP2), dt);
    }
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

    // Detect events by comparing current and previous state snapshots.
    if (m_state.m_score1 != m_prevState.m_score1 || m_state.m_score2 != m_prevState.m_score2) {
        playScore();
    } else {
        // Ball position jump heuristic: if the ball moved more than half the
        // field in one snapshot, it likely bounced off a paddle or wall.
        const float dx = m_state.m_ball.m_x - m_prevState.m_ball.m_x;
        const float dy = m_state.m_ball.m_y - m_prevState.m_ball.m_y;
        if ((dx * dx + dy * dy) > 0.04F) {
            playHit();
        }
    }
    m_prevState = m_state;
}

// ── Shared ───────────────────────────────────────────────────────────────────

Direction PongLayer::localDir() const
{
    // Keyboard
    if (m_window.isKeyPressed(engine::Key::W) || m_window.isKeyPressed(engine::Key::Up)) {
        return Direction::Up;
    }
    if (m_window.isKeyPressed(engine::Key::S) || m_window.isKeyPressed(engine::Key::Down)) {
        return Direction::Down;
    }

    // Gamepad (use cached slot from onUpdate)
    if (m_gamepadSlot >= 0) {
        // D-pad
        if (m_window.isGamepadButtonPressed(engine::GamepadButton::DpadUp, m_gamepadSlot)) {
            return Direction::Up;
        }
        if (m_window.isGamepadButtonPressed(engine::GamepadButton::DpadDown, m_gamepadSlot)) {
            return Direction::Down;
        }

        // Left stick (with dead zone)
        static constexpr float kStickDeadZone = 0.3F;
        const float stickY = m_window.gamepadAxis(engine::GamepadAxis::LeftY, m_gamepadSlot);
        if (stickY < -kStickDeadZone) {
            return Direction::Up;
        }
        if (stickY > kStickDeadZone) {
            return Direction::Down;
        }
    }

    return Direction::Idle;
}

void PongLayer::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram dg = m_socket->receiveDatagram();

        if (m_role == Role::Host) {
            if (!m_clientConnected) {
                m_clientConnected = true;
                resetGame();
                m_phase = Phase::Playing;
            }
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
