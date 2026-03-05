#pragma once

#include <glad/glad.h>

#include <utility>

namespace engine
{

// RAII wrapper for OpenGL object handles.
// Deleter is a function pointer or callable passed as a non-type template
// parameter (C++20 NTTP), so each GL object type gets its own zero-overhead
// wrapper with no virtual dispatch and no heap allocation.
//
// Usage:
//   using ShaderProgram = GlObject<deleteProgram>;
//   ShaderProgram prog{glCreateProgram()};
template<auto Deleter>
class GlObject
{
public:
    explicit GlObject(GLuint id)
        : m_id(id)
    {
    }

    ~GlObject()
    {
        if (m_id != 0) {
            Deleter(m_id);
        }
    }

    GlObject(GlObject &&other) noexcept
        : m_id(std::exchange(other.m_id, 0))
    {
    }

    GlObject &operator=(GlObject &&other) noexcept
    {
        if (this != &other) {
            if (m_id != 0) {
                Deleter(m_id);
            }
            m_id = std::exchange(other.m_id, 0);
        }
        return *this;
    }

    GlObject(const GlObject &) = delete;
    GlObject &operator=(const GlObject &) = delete;

    GLuint id() const
    {
        return m_id;
    }

    explicit operator bool() const
    {
        return m_id != 0;
    }

private:
    GLuint m_id = 0;
};

// ── Deleters ──────────────────────────────────────────────────────────────────

inline void deleteProgram(GLuint id)
{
    glDeleteProgram(id);
}

inline void deleteShader(GLuint id)
{
    glDeleteShader(id);
}

inline void deleteBuffer(GLuint id)
{
    glDeleteBuffers(1, &id);
}

inline void deleteVao(GLuint id)
{
    glDeleteVertexArrays(1, &id);
}

// ── Aliases ───────────────────────────────────────────────────────────────────

using ShaderProgram = GlObject<deleteProgram>;
using GlShader = GlObject<deleteShader>;
using GlBuffer = GlObject<deleteBuffer>;
using Vao = GlObject<deleteVao>;

} // namespace engine
