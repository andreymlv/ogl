#include "flappylayer.h"

#include "ai.h"
#include "mlai.h"

#include <QFile>

#include <cmath>

FlappyLayer::FlappyLayer(engine::Window &window, engine::AudioEngine &audio)
    : engine::Layer(QStringLiteral("flappy"))
    , m_window(window)
    , m_audio(audio)
{
}

std::shared_ptr<engine::Texture> FlappyLayer::loadTexture(const QString &name)
{
    auto tex = std::make_shared<engine::Texture>();
    QFile file(QStringLiteral(":/") + name);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        tex->initFromData(reinterpret_cast<const unsigned char *>(data.constData()), static_cast<int>(data.size()));
    }
    return tex;
}

std::unique_ptr<engine::AudioClip> FlappyLayer::loadAudio(const QString &name)
{
    QFile file(QStringLiteral(":/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    const QByteArray data = file.readAll();
    return m_audio.createSoundFromData({reinterpret_cast<const unsigned char *>(data.constData()),
                                        static_cast<std::size_t>(data.size())});
}

void FlappyLayer::onAttach()
{
    m_r2d.init();

    // Load textures from embedded Qt resources
    m_textures.background = loadTexture(QStringLiteral("sprites/background-day.png"));
    m_textures.base = loadTexture(QStringLiteral("sprites/base.png"));
    m_textures.pipe = loadTexture(QStringLiteral("sprites/pipe-green.png"));
    m_textures.bird[0] = loadTexture(QStringLiteral("sprites/yellowbird-downflap.png"));
    m_textures.bird[1] = loadTexture(QStringLiteral("sprites/yellowbird-midflap.png"));
    m_textures.bird[2] = loadTexture(QStringLiteral("sprites/yellowbird-upflap.png"));
    m_textures.message = loadTexture(QStringLiteral("sprites/message.png"));
    m_textures.gameover = loadTexture(QStringLiteral("sprites/gameover.png"));
    for (int i = 0; i < 10; ++i) {
        m_textures.digitTex[static_cast<std::size_t>(i)] =
            loadTexture(QStringLiteral("sprites/%1.png").arg(i));
    }

    // Load audio clips from embedded Qt resources
    m_wingClip = loadAudio(QStringLiteral("audio/wing.wav"));
    m_pointClip = loadAudio(QStringLiteral("audio/point.wav"));
    m_hitClip = loadAudio(QStringLiteral("audio/hit.wav"));
    m_dieClip = loadAudio(QStringLiteral("audio/die.wav"));
    m_swooshClip = loadAudio(QStringLiteral("audio/swoosh.wav"));

    // Load ML model from embedded Qt resource
    QFile modelFile(QStringLiteral(":/model.onnx"));
    if (modelFile.open(QIODevice::ReadOnly)) {
        const QByteArray modelData = modelFile.readAll();
        m_mlModel.initFromData(reinterpret_cast<const unsigned char *>(modelData.constData()),
                               static_cast<std::size_t>(modelData.size()));
    }

    // Build scene
    m_entities = buildFlappyScene(m_scene, m_textures);

    // Initialize pipe positions
    std::uniform_real_distribution<float> dist(kPipeMinGapY, kPipeMaxGapY);
    for (int i = 0; i < kPipeCount; ++i) {
        m_pipeState[static_cast<std::size_t>(i)].x = kWorldW + static_cast<float>(i) * kPipeSpacing;
        m_pipeState[static_cast<std::size_t>(i)].gapCenterY = dist(m_rng);
    }
}

void FlappyLayer::onDetach()
{
    m_wingClip.reset();
    m_pointClip.reset();
    m_hitClip.reset();
    m_dieClip.reset();
    m_swooshClip.reset();
}

void FlappyLayer::onUpdate(float dt)
{
    // Input edge detection
    const bool spaceDown = m_window.isKeyPressed(engine::Key::Space);
    const bool spacePressed = spaceDown && !m_spaceWasPressed;
    m_spaceWasPressed = spaceDown;

    // AI toggle — A key edge detection
    const bool aDown = m_window.isKeyPressed(engine::Key::A);
    const bool aPressed = aDown && !m_aWasPressed;
    m_aWasPressed = aDown;
    if (aPressed) {
        switch (m_aiMode) {
        case AiMode::Off:
            m_aiMode = AiMode::Heuristic;
            break;
        case AiMode::Heuristic:
            m_aiMode = m_mlModel.isValid() ? AiMode::ML : AiMode::Off;
            break;
        case AiMode::ML:
            m_aiMode = AiMode::Off;
            break;
        }
    }

    // Bird animation — always active
    m_animTimer += dt;
    if (m_animTimer >= kAnimInterval) {
        m_animTimer -= kAnimInterval;
        m_birdFrame = (m_birdFrame + 1) % 3;
    }

    // Ground scroll — always in Ready and Playing
    if (m_phase != GamePhase::GameOver) {
        m_baseOffset += kPipeSpeed * dt;
        if (m_baseOffset >= kBaseW) {
            m_baseOffset -= kBaseW;
        }
    }

    switch (m_phase) {
    case GamePhase::Ready: {
        // Bob bird up and down
        m_bobTimer += dt;
        m_birdY = kBirdStartY + std::sin(m_bobTimer * 3.F) * 8.F;
        m_birdVy = 0.F;

        if (spacePressed || m_aiMode != AiMode::Off) {
            m_phase = GamePhase::Playing;
            m_birdY = kBirdStartY;
            flap(m_birdVy);
            if (m_wingClip) {
                m_wingClip->play();
            }
            if (m_swooshClip) {
                m_swooshClip->play();
            }
        }
        break;
    }
    case GamePhase::Playing: {
        updateBird(m_birdY, m_birdVy, dt);
        updatePipes(m_pipeState, dt, m_rng);

        // Scoring
        const int scored = checkScore(kBirdX, m_pipeState);
        if (scored > 0) {
            m_score += scored;
            if (m_pointClip) {
                m_pointClip->play();
            }
        }

        // Collision
        if (checkCollisions(kBirdX, m_birdY, m_pipeState)) {
            m_phase = GamePhase::GameOver;
            m_gameOverTimer = 0.F;
            if (m_hitClip) {
                m_hitClip->play();
            }
            if (m_dieClip) {
                m_dieClip->play();
            }
        }

        bool aiFlap = false;
        if (m_aiMode == AiMode::Heuristic) {
            aiFlap = aiShouldFlap(m_birdY, m_birdVy, m_pipeState);
        } else if (m_aiMode == AiMode::ML && m_mlModel.isValid()) {
            aiFlap = mlShouldFlap(m_mlModel, m_birdY, m_birdVy, m_pipeState);
        }
        if (spacePressed || aiFlap) {
            flap(m_birdVy);
            if (m_wingClip) {
                m_wingClip->play();
            }
        }
        break;
    }
    case GamePhase::GameOver: {
        // Bird keeps falling
        if (m_birdY > kFloorY) {
            updateBird(m_birdY, m_birdVy, dt);
            if (m_birdY <= kFloorY) {
                m_birdY = kFloorY;
                m_birdVy = 0.F;
            }
        }

        m_gameOverTimer += dt;
        if ((spacePressed || m_aiMode != AiMode::Off) && m_gameOverTimer > 0.5F) {
            // Reset game
            m_phase = GamePhase::Ready;
            m_birdY = kBirdStartY;
            m_birdVy = 0.F;
            m_score = 0;
            m_bobTimer = 0.F;
            m_gameOverTimer = 0.F;

            std::uniform_real_distribution<float> dist(kPipeMinGapY, kPipeMaxGapY);
            for (int i = 0; i < kPipeCount; ++i) {
                m_pipeState[static_cast<std::size_t>(i)].x = kWorldW + static_cast<float>(i) * kPipeSpacing;
                m_pipeState[static_cast<std::size_t>(i)].gapCenterY = dist(m_rng);
                m_pipeState[static_cast<std::size_t>(i)].scored = false;
            }

            if (m_swooshClip) {
                m_swooshClip->play();
            }
        }
        break;
    }
    }

    // Sync pipe state to scene entities
    for (int i = 0; i < kPipeCount; ++i) {
        m_entities.pipes[static_cast<std::size_t>(i)].x = m_pipeState[static_cast<std::size_t>(i)].x;
        m_entities.pipes[static_cast<std::size_t>(i)].gapCenterY = m_pipeState[static_cast<std::size_t>(i)].gapCenterY;
    }

    syncFlappyScene(m_entities, m_textures,
                    m_birdY, birdRotation(m_birdVy), m_birdFrame,
                    m_baseOffset, m_phase, m_score);
}

void FlappyLayer::onRender()
{
    m_scene.onRender(m_r2d, m_cam);
}
