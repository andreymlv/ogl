//
// learnopengl.com — Getting Started / Hello Triangle
// https://learnopengl.com/Getting-started/Hello-Triangle
//
// Draws a single orange triangle using the engine's Application loop,
// Shader class, and GlObject RAII wrappers (VAO + VBO).
//

#include <application.h>
#include <glfwwindow.h>
#include <globject.h>
#include <openglrenderer.h>
#include <shader.h>

#include <glad/glad.h>

#include <QCoreApplication>
#include <QLoggingCategory>

// ── Shaders ───────────────────────────────────────────────────────────────────

static constexpr const char *kVertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)glsl";

static constexpr const char *kFragSrc = R"glsl(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)glsl";

// ── Triangle vertices (NDC) ───────────────────────────────────────────────────

// clang-format off
static constexpr float kVertices[] = {
    -0.5F, -0.5F, 0.0F,   // bottom-left
     0.5F, -0.5F, 0.0F,   // bottom-right
     0.0F,  0.5F, 0.0F,   // top-centre
};
// clang-format on

// ── TriangleApp ───────────────────────────────────────────────────────────────

class TriangleApp : public engine::Application
{
public:
    // GPU resources are created in onInit(), not here, because an OpenGL
    // context only exists after Application::init() has run.
    bool onInit()
    {
        if (!m_shader.init(kVertSrc, kFragSrc)) {
            return false;
        }

        // VAO records all vertex attribute state; create it first.
        m_vao = engine::Vao{[]() {
            GLuint id = 0;
            glGenVertexArrays(1, &id);
            return id;
        }()};
        glBindVertexArray(m_vao.id());

        // VBO uploads vertex data to GPU memory.
        m_vbo = engine::GlBuffer{[]() {
            GLuint id = 0;
            glGenBuffers(1, &id);
            return id;
        }()};
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
        glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        // Tell the vertex shader how to read attribute 0 (aPos): 3 floats,
        // tightly packed, starting at offset 0.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        // Unbind — good practice; the VAO remembers the state.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return true;
    }

protected:
    void onRender() override
    {
        m_shader.bind();
        glBindVertexArray(m_vao.id());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    engine::Shader m_shader;
    engine::Vao m_vao{0};
    engine::GlBuffer m_vbo{0};
};

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication qtApp(argc, argv);

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, "Hello Triangle")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();

    TriangleApp app;
    if (!app.init(std::move(window), std::move(renderer))) {
        return 1;
    }

    // Upload GPU resources now that the OpenGL context is active.
    if (!app.onInit()) {
        return 1;
    }

    return app.run();
}
