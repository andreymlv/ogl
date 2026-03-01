#include "renderer2d.h"

#include "globject.h"
#include "shader.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <QLoggingCategory>

#include "engine_logging.h"

#include <array>
#include <vector>

namespace engine
{

// ── Vertex layout ─────────────────────────────────────────────────────────────

struct Vertex {
    glm::vec2 position;
    glm::vec4 color;
};

static constexpr int kMaxQuads = 1000;
static constexpr int kVerticesPerQuad = 4;
static constexpr int kIndicesPerQuad = 6;

// ── Shaders ───────────────────────────────────────────────────────────────────

static constexpr const char *kVertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

out vec4 vColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)glsl";

static constexpr const char *kFragSrc = R"glsl(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)glsl";

// ── Private ───────────────────────────────────────────────────────────────────

class Renderer2DPrivate
{
public:
    Shader m_shader;
    Vao m_vao{0};
    GlBuffer m_vbo{0};
    GlBuffer m_ebo{0};

    std::vector<Vertex> m_vertices;
    int m_quadCount = 0;
    bool m_valid = false;
};

// ── Renderer2D ────────────────────────────────────────────────────────────────

Renderer2D::Renderer2D()
    : d(std::make_unique<Renderer2DPrivate>())
{
}

Renderer2D::~Renderer2D() = default;

bool Renderer2D::init()
{
    if (!d->m_shader.init(kVertSrc, kFragSrc)) {
        return false;
    }

    // Pre-compute the index buffer — layout never changes, only vertex data
    // varies per frame.  Two triangles per quad: 0-1-2, 2-3-0.
    std::array<GLuint, kMaxQuads * kIndicesPerQuad> indices{};
    for (int q = 0; q < kMaxQuads; ++q) {
        const int vi = q * kVerticesPerQuad; // first vertex of this quad
        const int ii = q * kIndicesPerQuad; // first index slot
        indices[ii + 0] = static_cast<GLuint>(vi + 0);
        indices[ii + 1] = static_cast<GLuint>(vi + 1);
        indices[ii + 2] = static_cast<GLuint>(vi + 2);
        indices[ii + 3] = static_cast<GLuint>(vi + 2);
        indices[ii + 4] = static_cast<GLuint>(vi + 3);
        indices[ii + 5] = static_cast<GLuint>(vi + 0);
    }

    GLuint vaoId = 0;
    glGenVertexArrays(1, &vaoId);
    d->m_vao = Vao{vaoId};
    glBindVertexArray(d->m_vao.id());

    GLuint vboId = 0;
    glGenBuffers(1, &vboId);
    d->m_vbo = GlBuffer{vboId};
    glBindBuffer(GL_ARRAY_BUFFER, d->m_vbo.id());
    // Allocate max capacity; data is uploaded per-frame with GL_DYNAMIC_DRAW.
    glBufferData(GL_ARRAY_BUFFER, kMaxQuads * kVerticesPerQuad * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // position — location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // color — location 1
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);

    GLuint eboId = 0;
    glGenBuffers(1, &eboId);
    d->m_ebo = GlBuffer{eboId};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->m_ebo.id());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    d->m_vertices.reserve(kMaxQuads * kVerticesPerQuad);
    d->m_valid = true;
    return true;
}

bool Renderer2D::isValid() const
{
    return d->m_valid;
}

void Renderer2D::beginScene()
{
    d->m_vertices.clear();
    d->m_quadCount = 0;
}

void Renderer2D::drawQuad(glm::vec2 position, glm::vec2 size, glm::vec4 color)
{
    // Four corners: bottom-left, bottom-right, top-right, top-left.
    d->m_vertices.push_back({position, color});
    d->m_vertices.push_back({{position.x + size.x, position.y}, color});
    d->m_vertices.push_back({{position.x + size.x, position.y + size.y}, color});
    d->m_vertices.push_back({{position.x, position.y + size.y}, color});
    ++d->m_quadCount;
}

void Renderer2D::endScene()
{
    if (d->m_quadCount == 0) {
        return;
    }

    // Upload this frame's vertices into the VBO.
    glBindVertexArray(d->m_vao.id());
    glBindBuffer(GL_ARRAY_BUFFER, d->m_vbo.id());
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(d->m_vertices.size() * sizeof(Vertex)), d->m_vertices.data());

    d->m_shader.bind();
    glDrawElements(GL_TRIANGLES, d->m_quadCount * kIndicesPerQuad, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

int Renderer2D::quadCount() const
{
    return d->m_quadCount;
}

} // namespace engine
