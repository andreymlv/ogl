#include <QTest>

#include "camera2d.h"
#include "renderer2d.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ── GL context fixture ────────────────────────────────────────────────────────

class Renderer2DTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // init()
    void defaultConstructedIsInvalid();
    void initReturnsTrue();
    void initTwiceReturnsTrue(); // idempotent re-init

    // beginScene / endScene
    void quadCountIsZeroAfterBeginScene();
    void endSceneWithoutQuadsDoesNotCrash();

    // drawQuad
    void drawQuadIncrementsCount();
    void drawMultipleQuadsAccumulatesCount();

    // endScene resets count
    void quadCountResetsOnNextBeginScene();

private:
    GLFWwindow *m_ctx = nullptr;
    // Unit ortho camera — NDC space, no transformation.
    engine::Camera2D m_cam{-1.F, 1.F, -1.F, 1.F};
};

// ── fixture ───────────────────────────────────────────────────────────────────

void Renderer2DTest::initTestCase()
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

void Renderer2DTest::cleanupTestCase()
{
    if (m_ctx) {
        glfwDestroyWindow(m_ctx);
    }
    glfwTerminate();
}

// ── tests ─────────────────────────────────────────────────────────────────────

void Renderer2DTest::defaultConstructedIsInvalid()
{
    engine::Renderer2D r;
    QVERIFY(!r.isValid());
}

void Renderer2DTest::initReturnsTrue()
{
    engine::Renderer2D r;
    QVERIFY(r.init());
    QVERIFY(r.isValid());
}

void Renderer2DTest::initTwiceReturnsTrue()
{
    engine::Renderer2D r;
    QVERIFY(r.init());
    QVERIFY(r.init()); // must not crash or return false
    QVERIFY(r.isValid());
}

void Renderer2DTest::quadCountIsZeroAfterBeginScene()
{
    engine::Renderer2D r;
    r.init();
    r.beginScene(m_cam);
    QCOMPARE(r.quadCount(), 0);
}

void Renderer2DTest::endSceneWithoutQuadsDoesNotCrash()
{
    engine::Renderer2D r;
    r.init();
    r.beginScene(m_cam);
    r.endScene(); // must not crash, draw call with 0 quads is valid
}

void Renderer2DTest::drawQuadIncrementsCount()
{
    engine::Renderer2D r;
    r.init();
    r.beginScene(m_cam);
    r.drawQuad({0.F, 0.F}, {1.F, 1.F}, {1.F, 0.F, 0.F, 1.F});
    QCOMPARE(r.quadCount(), 1);
}

void Renderer2DTest::drawMultipleQuadsAccumulatesCount()
{
    engine::Renderer2D r;
    r.init();
    r.beginScene(m_cam);
    r.drawQuad({0.F, 0.F}, {1.F, 1.F}, {1.F, 0.F, 0.F, 1.F});
    r.drawQuad({2.F, 0.F}, {1.F, 1.F}, {0.F, 1.F, 0.F, 1.F});
    r.drawQuad({4.F, 0.F}, {1.F, 1.F}, {0.F, 0.F, 1.F, 1.F});
    QCOMPARE(r.quadCount(), 3);
}

void Renderer2DTest::quadCountResetsOnNextBeginScene()
{
    engine::Renderer2D r;
    r.init();

    r.beginScene(m_cam);
    r.drawQuad({0.F, 0.F}, {1.F, 1.F}, {1.F, 1.F, 1.F, 1.F});
    r.endScene();

    r.beginScene(m_cam); // new frame — count must reset
    QCOMPARE(r.quadCount(), 0);
}

QTEST_GUILESS_MAIN(Renderer2DTest)
#include "renderer2dtest.moc"
