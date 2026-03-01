#include <QTest>

#include "glfwwindow.h"
#include "globject.h"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ── GL context fixture ────────────────────────────────────────────────────────
//
// Shader compilation and GlObject RAII both require an active OpenGL context.
// We create a hidden 1×1 GLFW window once for the whole test suite and destroy
// it in cleanupTestCase().  All test slots run inside that context.

namespace
{

static constexpr const char *kVertOk = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
void main() { gl_Position = vec4(aPos, 1.0); }
)glsl";

static constexpr const char *kFragOk = R"glsl(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0, 0.5, 0.2, 1.0); }
)glsl";

static constexpr const char *kVertBad = "not glsl at all";
static constexpr const char *kFragBad = "also broken";

} // namespace

class ShaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // Shader — construction
    void defaultConstructedIsInvalid();

    // Shader — init() failure paths
    void initWithBadVertReturnsFalse();
    void initWithBadFragReturnsFalse();

    // Shader — init() success
    void initWithValidSrcReturnsTrue();
    void initTwiceReplacesProgram();

    // Shader — bind
    void bindMakesShaderCurrent();

    // GlObject — move semantics
    void glObjectMoveTransfersOwnership();
    void glObjectMoveAssignTransfersOwnership();

private:
    GLFWwindow *m_ctx = nullptr;
};

// ── fixture ───────────────────────────────────────────────────────────────────

void ShaderTest::initTestCase()
{
    QVERIFY(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // headless — no window on screen

    m_ctx = glfwCreateWindow(1, 1, "test", nullptr, nullptr);
    QVERIFY(m_ctx);
    glfwMakeContextCurrent(m_ctx);

    QVERIFY(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
}

void ShaderTest::cleanupTestCase()
{
    if (m_ctx) {
        glfwDestroyWindow(m_ctx);
    }
    glfwTerminate();
}

// ── Shader tests ──────────────────────────────────────────────────────────────

void ShaderTest::defaultConstructedIsInvalid()
{
    engine::Shader s;
    QVERIFY(!s.isValid());
}

void ShaderTest::initWithBadVertReturnsFalse()
{
    engine::Shader s;
    QVERIFY(!s.init(kVertBad, kFragOk));
    QVERIFY(!s.isValid());
}

void ShaderTest::initWithBadFragReturnsFalse()
{
    engine::Shader s;
    QVERIFY(!s.init(kVertOk, kFragBad));
    QVERIFY(!s.isValid());
}

void ShaderTest::initWithValidSrcReturnsTrue()
{
    engine::Shader s;
    QVERIFY(s.init(kVertOk, kFragOk));
    QVERIFY(s.isValid());
}

void ShaderTest::initTwiceReplacesProgram()
{
    engine::Shader s;
    QVERIFY(s.init(kVertOk, kFragOk));
    const GLuint first = [&] {
        s.bind();
        GLint id = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &id);
        return static_cast<GLuint>(id);
    }();

    QVERIFY(s.init(kVertOk, kFragOk)); // second init — should replace program
    QVERIFY(s.isValid());

    s.bind();
    GLint second = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &second);

    // A new program object must have been allocated (different id).
    QVERIFY(static_cast<GLuint>(second) != first);
}

void ShaderTest::bindMakesShaderCurrent()
{
    engine::Shader s;
    QVERIFY(s.init(kVertOk, kFragOk));

    s.bind();

    GLint current = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    QVERIFY(current != 0);
}

// ── GlObject tests ────────────────────────────────────────────────────────────

void ShaderTest::glObjectMoveTransfersOwnership()
{
    GLuint id = 0;
    glGenBuffers(1, &id);
    QVERIFY(id != 0);

    engine::GlBuffer a{id};
    QCOMPARE(a.id(), id);

    engine::GlBuffer b{std::move(a)};
    QCOMPARE(b.id(), id);
    QCOMPARE(a.id(), 0u); // a gave up ownership
}

void ShaderTest::glObjectMoveAssignTransfersOwnership()
{
    GLuint idA = 0;
    GLuint idB = 0;
    glGenBuffers(1, &idA);
    glGenBuffers(1, &idB);

    engine::GlBuffer a{idA};
    engine::GlBuffer b{idB};

    b = std::move(a);        // b's old buffer must be deleted, a zeroed
    QCOMPARE(b.id(), idA);
    QCOMPARE(a.id(), 0u);
    // idB was deleted by the move-assign; we just verify no crash / no leak
    // (AddressSanitizer + LeakSanitizer will catch any issues).
}

QTEST_GUILESS_MAIN(ShaderTest)
#include "shadertest.moc"
