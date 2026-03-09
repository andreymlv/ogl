#include "breakoutlayer.h"

#include <QFile>

#include <imgui.h>

#include <cmath>

BreakoutLayer::BreakoutLayer(engine::Window &window, engine::AudioEngine &audio)
    : engine::Layer(QStringLiteral("breakout"))
    , m_window(window)
    , m_audio(audio)
{
}

std::shared_ptr<engine::Texture> BreakoutLayer::loadTexture(const QString &name)
{
    auto tex = std::make_shared<engine::Texture>();
    QFile file(QStringLiteral(":/") + name);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        tex->initFromData(reinterpret_cast<const unsigned char *>(data.constData()), static_cast<int>(data.size()));
    }
    return tex;
}

void BreakoutLayer::onAttach()
{
    m_r2d.init();

    // Load textures
    m_textures.background = loadTexture(QStringLiteral("sprites/Background1.png"));
    m_textures.frame = loadTexture(QStringLiteral("sprites/Frame.png"));
    m_textures.paddle = loadTexture(QStringLiteral("sprites/Player.png"));
    m_textures.paddleFlash = loadTexture(QStringLiteral("sprites/Player_flash.png"));
    m_textures.ball = loadTexture(QStringLiteral("sprites/Ball_small-blue.png"));
    for (int i = 0; i < 9; ++i) {
        m_textures.brickTex[static_cast<std::size_t>(i)] = loadTexture(QStringLiteral("sprites/Brick%1_4.png").arg(i + 1));
    }

    // Generate audio clips (simple sine waves)
    const auto hitSamples = makeSine(440.F, 0.06F);
    m_hitClip = m_audio.createSound(hitSamples, kSampleRate);

    const auto brickSamples = makeSine(660.F, 0.05F);
    m_brickClip = m_audio.createSound(brickSamples, kSampleRate);

    const auto loseSamples = makeSine(220.F, 0.30F);
    m_loseClip = m_audio.createSound(loseSamples, kSampleRate);

    const auto wallSamples = makeSine(330.F, 0.04F);
    m_wallClip = m_audio.createSound(wallSamples, kSampleRate);

    // Build initial brick layout
    m_bricks = buildBricks();

    // Build scene
    m_entities = buildBreakoutScene(m_scene, m_textures, m_bricks);

    // Load persisted high score
    m_hiScore = m_settings.value(QStringLiteral("hiScore"), 0).toInt();
}

void BreakoutLayer::onDetach()
{
    m_hitClip.reset();
    m_brickClip.reset();
    m_loseClip.reset();
    m_wallClip.reset();
}

void BreakoutLayer::resetBall()
{
    m_ball = BallState{};
    m_ball.x = m_paddleX + m_paddleW / 2.F - kBallSize / 2.F;
    m_ball.y = kPaddleY + kPaddleH + 2.F;
    m_ball.launched = false;
    m_ceilingHit = false;
    m_paddleNarrow = false;
    m_paddleW = kPaddleW;
}

void BreakoutLayer::resetGame()
{
    m_score = 0;
    m_lives = kStartLives;
    m_ballSpeed = kBallSpeedInitial;
    m_ceilingHit = false;
    m_paddleNarrow = false;
    m_paddleW = kPaddleW;
    m_paddleX = kWorldW / 2.F - kPaddleW / 2.F;
    m_bricks = buildBricks();

    // Rebuild brick entities in the scene
    for (auto &e : m_entities.bricks) {
        m_scene.destroyEntity(e);
    }
    m_entities.bricks.clear();
    for (std::size_t i = 0; i < m_bricks.size(); ++i) {
        const auto &bs = m_bricks[i];
        auto e = m_scene.createEntity("brick");
        auto &tf = e.addComponent<engine::Transform>();
        tf.position = {bs.x(), bs.y()};
        tf.scale = {kBrickW, kBrickH};
        auto &sp = e.addComponent<engine::Sprite>();
        const auto texIdx = static_cast<std::size_t>(bs.row % 9);
        sp.texture = m_textures.brickTex[texIdx];
        sp.color = kRowColors[bs.row];
        sp.zOrder = 1;
        m_entities.bricks.push_back(e);
    }

    resetBall();
}

void BreakoutLayer::onUpdate(float dt)
{
    // Discover gamepad
    m_gamepadSlot = m_window.firstGamepadSlot();

    // Space / gamepad A / Start edge detection
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
    } else if ((m_phase == Phase::GameOver || m_phase == Phase::Victory) && spacePressed) {
        m_phase = Phase::Ready;
    }

    if (m_phase != Phase::Playing) {
        syncBreakoutScene(m_entities, m_textures, m_paddleX, m_paddleW, m_paddleNarrow, m_ball, m_bricks);
        return;
    }

    // ── Paddle input ────────────────────────────────────────────────────────
    float dir = 0.F;
    if (m_window.isKeyPressed(engine::Key::Left) || m_window.isKeyPressed(engine::Key::A)) {
        dir -= 1.F;
    }
    if (m_window.isKeyPressed(engine::Key::Right) || m_window.isKeyPressed(engine::Key::D)) {
        dir += 1.F;
    }

    // Gamepad
    if (m_gamepadSlot >= 0) {
        if (m_window.isGamepadButtonPressed(engine::GamepadButton::DpadLeft, m_gamepadSlot)) {
            dir -= 1.F;
        }
        if (m_window.isGamepadButtonPressed(engine::GamepadButton::DpadRight, m_gamepadSlot)) {
            dir += 1.F;
        }
        static constexpr float kStickDeadZone = 0.3F;
        const float stickX = m_window.gamepadAxis(engine::GamepadAxis::LeftX, m_gamepadSlot);
        if (stickX < -kStickDeadZone) {
            dir -= 1.F;
        } else if (stickX > kStickDeadZone) {
            dir += 1.F;
        }
    }
    dir = std::clamp(dir, -1.F, 1.F);

    m_paddleX = movePaddle(m_paddleX, dir, dt, m_paddleW);

    // ── Ball follows paddle before launch ───────────────────────────────────
    if (!m_ball.launched) {
        m_ball.x = m_paddleX + m_paddleW / 2.F - kBallSize / 2.F;
        m_ball.y = kPaddleY + kPaddleH + 2.F;

        if (spacePressed) {
            m_ball.launched = true;
            // Launch at 75 degrees upward with slight random horizontal bias
            m_ball.vx = m_ballSpeed * 0.25F;
            m_ball.vy = m_ballSpeed * 0.97F;
        }

        syncBreakoutScene(m_entities, m_textures, m_paddleX, m_paddleW, m_paddleNarrow, m_ball, m_bricks);
        return;
    }

    // ── Ball simulation ─────────────────────────────────────────────────────
    const SimResult result = simulateBall(m_ball, m_paddleX, m_paddleW, m_ballSpeed, m_bricks, dt);

    // Sound effects
    if (hasFlag(result.events, BreakoutEvent::PaddleBounce)) {
        if (m_hitClip) {
            m_hitClip->play();
        }
    }
    if (hasFlag(result.events, BreakoutEvent::BrickBounce)) {
        if (m_brickClip) {
            m_brickClip->play();
        }
    }
    if (hasFlag(result.events, BreakoutEvent::WallBounce)) {
        if (m_wallClip) {
            m_wallClip->play();
        }
    }
    if (hasFlag(result.events, BreakoutEvent::CeilingBounce)) {
        if (m_wallClip) {
            m_wallClip->play();
        }
        // Stretch goal: paddle narrows when ball hits ceiling
        if (!m_ceilingHit) {
            m_ceilingHit = true;
            m_paddleNarrow = true;
            m_paddleW = kPaddleNarrowW;
            // Re-center paddle if it would go out of bounds
            const float maxX = kWorldW - kWallThickness - m_paddleW;
            m_paddleX = std::clamp(m_paddleX, kWallThickness, maxX);
        }
    }

    // Score and speed increase
    if (result.bricksDestroyed > 0) {
        m_score += result.scoreGained;
        m_ballSpeed = std::min(m_ballSpeed + kBallSpeedIncrease * static_cast<float>(result.bricksDestroyed), kBallSpeedMax);

        // Normalize velocity to new speed
        const float currentSpeed = std::sqrt(m_ball.vx * m_ball.vx + m_ball.vy * m_ball.vy);
        if (currentSpeed > 0.F) {
            const float scale = m_ballSpeed / currentSpeed;
            m_ball.vx *= scale;
            m_ball.vy *= scale;
        }
    }

    // Ball lost
    if (hasFlag(result.events, BreakoutEvent::BallLost)) {
        --m_lives;
        if (m_loseClip) {
            m_loseClip->play();
        }
        if (m_lives <= 0) {
            m_phase = Phase::GameOver;
            // Update high score
            if (m_score > m_hiScore) {
                m_hiScore = m_score;
                m_settings.setValue(QStringLiteral("hiScore"), m_hiScore);
            }
        } else {
            resetBall();
        }
    }

    // Victory check
    if (countAliveBricks(m_bricks) == 0) {
        m_phase = Phase::Victory;
        // Update high score
        if (m_score > m_hiScore) {
            m_hiScore = m_score;
            m_settings.setValue(QStringLiteral("hiScore"), m_hiScore);
        }
    }

    syncBreakoutScene(m_entities, m_textures, m_paddleX, m_paddleW, m_paddleNarrow, m_ball, m_bricks);
}

void BreakoutLayer::onRender()
{
    m_scene.onRender(m_r2d, m_cam);
    renderImGui();
}

// ── ImGui HUD ───────────────────────────────────────────────────────────────

void BreakoutLayer::renderImGui()
{
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 center{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F};

    // Always show score and lives
    {
        ImGui::SetNextWindowPos({vp->WorkPos.x + 10.F, vp->WorkPos.y + 5.F}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5F);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("Score: %d", m_score);
        if (m_hiScore > 0) {
            ImGui::SameLine();
            ImGui::Text("  Hi: %d", m_hiScore);
        }
        ImGui::Text("Lives: %d", m_lives);
        ImGui::End();
    }

    // Phase overlays
    if (m_phase == Phase::Ready) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.7F);
        ImGui::Begin("Ready", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("BREAKOUT");
        ImGui::Separator();
        ImGui::Text("Left/Right or A/D to move");
        ImGui::Text("Space to launch ball");
        if (m_gamepadSlot >= 0) {
            ImGui::Text("D-pad / Left Stick to move");
            ImGui::Text("A or START to begin");
        } else {
            ImGui::Text("Press SPACE to start");
        }
        ImGui::End();
    } else if (m_phase == Phase::Playing && !m_ball.launched) {
        ImGui::SetNextWindowPos({center.x, vp->WorkPos.y + vp->WorkSize.y * 0.65F}, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.5F);
        ImGui::Begin("Launch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("Press SPACE to launch");
        ImGui::End();
    } else if (m_phase == Phase::GameOver) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.7F);
        ImGui::Begin("GameOver",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("GAME OVER");
        ImGui::Text("Final Score: %d", m_score);
        if (m_hiScore > 0) {
            ImGui::Text("High Score: %d", m_hiScore);
        }
        ImGui::Separator();
        ImGui::Text("Press SPACE to restart");
        ImGui::End();
    } else if (m_phase == Phase::Victory) {
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5F, 0.5F});
        ImGui::SetNextWindowBgAlpha(0.7F);
        ImGui::Begin("Victory",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        ImGui::Text("YOU WIN!");
        ImGui::Text("Final Score: %d", m_score);
        if (m_hiScore > 0) {
            ImGui::Text("High Score: %d", m_hiScore);
        }
        ImGui::Separator();
        ImGui::Text("Press SPACE to play again");
        ImGui::End();
    }
}
