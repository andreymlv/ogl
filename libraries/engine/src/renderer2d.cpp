#include "renderer2d.h"

#include "camera2d.h"
#include "globject.h"
#include "shader.h"
#include "texture.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <QLoggingCategory>

#include "engine_logging.h"

#include <array>
#include <vector>

namespace engine
{

// ── Vertex layout ─────────────────────────────────────────────────────────────
//
// Every vertex carries position, color, UV and a texture-slot index.
// drawQuad fills uv with zeros and texIndex with -1 (pure color, no sample).
// drawSprite fills uv with corner UVs and texIndex with the slot assigned to
// the texture for this batch.

struct Vertex {
    glm::vec2 position;
    glm::vec4 color;
    glm::vec2 uv;
    float texIndex; // -1 = color quad, 0..N = texture slot
};

static constexpr int kMaxQuads = 1000;
static constexpr int kVerticesPerQuad = 4;
static constexpr int kIndicesPerQuad = 6;
static constexpr int kMaxTextureSlots = 16;

// ── Shaders ───────────────────────────────────────────────────────────────────

static constexpr const char *kVertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;
layout(location = 3) in float aTexIndex;

uniform mat4 uViewProjection;

out vec4 vColor;
out vec2 vUV;
out float vTexIndex;

void main()
{
    gl_Position = uViewProjection * vec4(aPos, 0.0, 1.0);
    vColor    = aColor;
    vUV       = aUV;
    vTexIndex = aTexIndex;
}
)glsl";

static constexpr const char *kFragSrc = R"glsl(
#version 330 core
in vec4  vColor;
in vec2  vUV;
in float vTexIndex;

uniform sampler2D uTextures[16];

out vec4 FragColor;

void main()
{
    int slot = int(vTexIndex);
    if (slot < 0) {
        FragColor = vColor;
    } else {
        FragColor = texture(uTextures[slot], vUV) * vColor;
    }
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

    std::array<GLuint, kMaxTextureSlots> m_textureSlots{};
    int m_textureSlotCount = 0;
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

    // Bind sampler uniforms once — slots 0..15 map to uTextures[0..15].
    d->m_shader.bind();
    for (int i = 0; i < kMaxTextureSlots; ++i) {
        d->m_shader.setUniform("uTextures[" + std::to_string(i) + "]", i);
    }

    // Pre-compute the index buffer — layout never changes, only vertex data
    // varies per frame.  Two triangles per quad: 0-1-2, 2-3-0.
    std::array<GLuint, kMaxQuads * kIndicesPerQuad> indices{};
    for (int q = 0; q < kMaxQuads; ++q) {
        const int vi = q * kVerticesPerQuad;
        const int ii = q * kIndicesPerQuad;
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
    glBufferData(GL_ARRAY_BUFFER, kMaxQuads * kVerticesPerQuad * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // position — location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // color — location 1
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);

    // uv — location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);

    // texIndex — location 3
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, texIndex)));
    glEnableVertexAttribArray(3);

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

void Renderer2D::beginScene(const Camera2D &camera)
{
    d->m_shader.bind();
    d->m_shader.setUniform("uViewProjection", camera.viewProjectionMatrix());
    d->m_vertices.clear();
    d->m_quadCount = 0;
    d->m_textureSlotCount = 0;
}

void Renderer2D::drawQuad(glm::vec2 position, glm::vec2 size, glm::vec4 color)
{
    // texIndex = -1 → fragment shader uses vColor directly, no texture sample.
    d->m_vertices.push_back({position, color, {0.F, 0.F}, -1.F});
    d->m_vertices.push_back({{position.x + size.x, position.y}, color, {1.F, 0.F}, -1.F});
    d->m_vertices.push_back({{position.x + size.x, position.y + size.y}, color, {1.F, 1.F}, -1.F});
    d->m_vertices.push_back({{position.x, position.y + size.y}, color, {0.F, 1.F}, -1.F});
    ++d->m_quadCount;
}

void Renderer2D::drawSprite(glm::vec2 position, glm::vec2 size, const Texture &texture)
{
    // Find or assign a texture slot for this texture within the current batch.
    const GLuint texId = texture.id();
    float slot = -1.F;
    for (int i = 0; i < d->m_textureSlotCount; ++i) {
        if (d->m_textureSlots[i] == texId) {
            slot = static_cast<float>(i);
            break;
        }
    }
    if (slot < 0.F) {
        if (d->m_textureSlotCount >= kMaxTextureSlots) {
            qCWarning(Engine) << "Renderer2D: texture slot limit reached, flushing not yet supported";
            return;
        }
        slot = static_cast<float>(d->m_textureSlotCount);
        d->m_textureSlots[d->m_textureSlotCount++] = texId;
    }

    static constexpr glm::vec4 kWhite{1.F, 1.F, 1.F, 1.F};
    d->m_vertices.push_back({position, kWhite, {0.F, 0.F}, slot});
    d->m_vertices.push_back({{position.x + size.x, position.y}, kWhite, {1.F, 0.F}, slot});
    d->m_vertices.push_back({{position.x + size.x, position.y + size.y}, kWhite, {1.F, 1.F}, slot});
    d->m_vertices.push_back({{position.x, position.y + size.y}, kWhite, {0.F, 1.F}, slot});
    ++d->m_quadCount;
}

void Renderer2D::endScene()
{
    if (d->m_quadCount == 0) {
        return;
    }

    // Bind all textures used this batch to their respective units.
    for (int i = 0; i < d->m_textureSlotCount; ++i) {
        glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + i));
        glBindTexture(GL_TEXTURE_2D, d->m_textureSlots[i]);
    }

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
