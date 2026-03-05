//
// learnopengl.com — Getting Started / Textures
// https://learnopengl.com/Getting-started/Textures
//
// Draws a textured quad that mixes two textures (container + awesomeface)
// using GLSL mix() — 80% container, 20% face.
// Uses engine's Shader, GlObject RAII and Texture for loading/uploading.
//

#include <application.h>
#include <miniaudioengine.h>
#include <glfwwindow.h>
#include <globject.h>
#include <openglrenderer.h>
#include <shader.h>
#include <texture.h>

#include <glad/glad.h>

#include <QCoreApplication>
#include <QFile>

// ── Shaders ───────────────────────────────────────────────────────────────────

static constexpr const char *kVertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord    = aTexCoord;
}
)glsl";

// Mix 80% container + 20% face, exactly as in the tutorial.
static constexpr const char *kFragSrc = R"glsl(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
)glsl";

// ── Quad vertices: position (3f) + UV (2f) ────────────────────────────────────

// clang-format off
static constexpr float kVertices[] = {
    // positions          // texture coords
     0.5F,  0.5F, 0.0F,  1.0F, 1.0F,  // top-right
     0.5F, -0.5F, 0.0F,  1.0F, 0.0F,  // bottom-right
    -0.5F, -0.5F, 0.0F,  0.0F, 0.0F,  // bottom-left
    -0.5F,  0.5F, 0.0F,  0.0F, 1.0F,  // top-left
};

static constexpr unsigned int kIndices[] = {
    0, 1, 3,  // first triangle
    1, 2, 3,  // second triangle
};
// clang-format on

// ── TextureApp ────────────────────────────────────────────────────────────────

class TextureApp : public engine::Application
{
public:
    bool onInit()
    {
        if (!m_shader.init(kVertSrc, kFragSrc)) {
            return false;
        }

        auto loadTexture = [](engine::Texture &tex, const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                return false;
            }
            const QByteArray data = file.readAll();
            return tex.initFromData(reinterpret_cast<const unsigned char *>(data.constData()),
                                    static_cast<int>(data.size()));
        };

        if (!loadTexture(m_texture1, QStringLiteral(":/container.jpg"))) {
            return false;
        }
        if (!loadTexture(m_texture2, QStringLiteral(":/awesomeface.png"))) {
            return false;
        }

        // Bind sampler uniforms: texture1 → unit 0, texture2 → unit 1.
        m_shader.bind();
        m_shader.setUniform("texture1", 0);
        m_shader.setUniform("texture2", 1);

        // VAO records all vertex attribute and element buffer state.
        GLuint vaoId = 0;
        glGenVertexArrays(1, &vaoId);
        m_vao = engine::Vao{vaoId};
        glBindVertexArray(m_vao.id());

        GLuint vboId = 0;
        glGenBuffers(1, &vboId);
        m_vbo = engine::GlBuffer{vboId};
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
        glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        GLuint eboId = 0;
        glGenBuffers(1, &eboId);
        m_ebo = engine::GlBuffer{eboId};
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.id());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

        // aPos — location 0: 3 floats, stride = 5 floats
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        // aTexCoord — location 1: 2 floats, offset = 3 floats
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<const void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        return true;
    }

protected:
    void onRender() override
    {
        m_texture1.bind(0);
        m_texture2.bind(1);

        m_shader.bind();
        glBindVertexArray(m_vao.id());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

private:
    engine::Shader m_shader;
    engine::Texture m_texture1;
    engine::Texture m_texture2;
    engine::Vao m_vao{0};
    engine::GlBuffer m_vbo{0};
    engine::GlBuffer m_ebo{0};
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
    if (!window->init(800, 600, "Textures")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();

    TextureApp app;
    auto audio = std::make_unique<engine::MiniaudioEngine>();
    if (!app.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }
    if (!app.onInit()) {
        return 1;
    }

    return app.run();
}
