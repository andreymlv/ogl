#include <QTest>

#include "camera2d.h"
#include "renderer2d.h"
#include "texture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ── GL context fixture ────────────────────────────────────────────────────────
//
// Texture loading and GL object RAII both require an active OpenGL context.
// A hidden 1×1 GLFW window is created once for the whole suite.

static QString testImage()
{
    return QString::fromLocal8Bit(qgetenv("PROJECT_SOURCE_DIR")) + QStringLiteral("/libraries/engine/autotests/container.jpg");
}

class TextureTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // Texture — construction
    void defaultConstructedIsInvalid();

    // Texture — init() failure
    void initWithBadPathReturnsFalse();
    void initWithBadPathLeavesInvalid();

    // Texture — init() success
    void initWithValidPathReturnsTrue();
    void initWithValidPathIsValid();

    // Texture — accessors
    void widthIsPositiveAfterInit();
    void heightIsPositiveAfterInit();

    // Texture — bind
    void bindMakesTextureCurrent();

    // Renderer2D — drawSprite
    void drawSpriteIncrementsQuadCount();

private:
    GLFWwindow *m_ctx = nullptr;
    engine::Camera2D m_cam{-1.F, 1.F, -1.F, 1.F};
};

// ── fixture ───────────────────────────────────────────────────────────────────

void TextureTest::initTestCase()
{
    QVERIFY(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_ctx = glfwCreateWindow(1, 1, "test", nullptr, nullptr);
    QVERIFY(m_ctx);
    glfwMakeContextCurrent(m_ctx);

    QVERIFY(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
}

void TextureTest::cleanupTestCase()
{
    if (m_ctx) {
        glfwDestroyWindow(m_ctx);
    }
    glfwTerminate();
}

// ── Texture tests ─────────────────────────────────────────────────────────────

void TextureTest::defaultConstructedIsInvalid()
{
    engine::Texture t;
    QVERIFY(!t.isValid());
}

void TextureTest::initWithBadPathReturnsFalse()
{
    engine::Texture t;
    QVERIFY(!t.init("/nonexistent/path/image.png"));
}

void TextureTest::initWithBadPathLeavesInvalid()
{
    engine::Texture t;
    t.init("/nonexistent/path/image.png");
    QVERIFY(!t.isValid());
}

void TextureTest::initWithValidPathReturnsTrue()
{
    engine::Texture t;
    QVERIFY(t.init(testImage().toStdString()));
}

void TextureTest::initWithValidPathIsValid()
{
    engine::Texture t;
    t.init(testImage().toStdString());
    QVERIFY(t.isValid());
}

void TextureTest::widthIsPositiveAfterInit()
{
    engine::Texture t;
    t.init(testImage().toStdString());
    QVERIFY(t.width() > 0);
}

void TextureTest::heightIsPositiveAfterInit()
{
    engine::Texture t;
    t.init(testImage().toStdString());
    QVERIFY(t.height() > 0);
}

void TextureTest::bindMakesTextureCurrent()
{
    engine::Texture t;
    QVERIFY(t.init(testImage().toStdString()));

    t.bind(0); // bind to texture unit 0

    GLint current = 0;
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current);
    QVERIFY(current != 0);
}

// ── Renderer2D — drawSprite ───────────────────────────────────────────────────

void TextureTest::drawSpriteIncrementsQuadCount()
{
    engine::Texture t;
    QVERIFY(t.init(testImage().toStdString()));

    engine::Renderer2D r;
    QVERIFY(r.init());

    r.beginScene(m_cam);
    r.drawSprite({0.F, 0.F}, {1.F, 1.F}, t);
    QCOMPARE(r.quadCount(), 1);
}

QTEST_GUILESS_MAIN(TextureTest)
#include "texturetest.moc"
