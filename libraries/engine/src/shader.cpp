#include "shader.h"

#include "globject.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QLoggingCategory>

#include "engine_logging.h"

#include <string>

namespace engine
{

// ── Private ───────────────────────────────────────────────────────────────────

class ShaderPrivate
{
public:
    ShaderProgram m_program{0};
};

// ── Helpers ───────────────────────────────────────────────────────────────────

static GlShader compileShader(GLenum type, std::string_view src)
{
    GlShader shader{glCreateShader(type)};
    const char *data = src.data();
    const auto length = static_cast<GLint>(src.size());
    glShaderSource(shader.id(), 1, &data, &length);
    glCompileShader(shader.id());

    GLint ok = 0;
    glGetShaderiv(shader.id(), GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        GLint logLen = 0;
        glGetShaderiv(shader.id(), GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::size_t>(logLen), '\0');
        glGetShaderInfoLog(shader.id(), logLen, nullptr, log.data());
        qCCritical(Engine) << "Shader compile error:" << log.c_str();
        return GlShader{0}; // shader destructor cleans up
    }
    qCDebug(Engine) << ((type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment") << "shader compiled";
    return shader;
}

// ── Shader ────────────────────────────────────────────────────────────────────

Shader::Shader()
    : d(std::make_unique<ShaderPrivate>())
{
}

Shader::~Shader() = default;

bool Shader::init(std::string_view vertSrc, std::string_view fragSrc)
{
    GlShader vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vert) {
        return false;
    }

    GlShader frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!frag) {
        return false;
    }

    ShaderProgram program{glCreateProgram()};
    glAttachShader(program.id(), vert.id());
    glAttachShader(program.id(), frag.id());
    glLinkProgram(program.id());

    // vert and frag go out of scope here — RAII deletes them automatically.

    GLint ok = 0;
    glGetProgramiv(program.id(), GL_LINK_STATUS, &ok);
    if (ok == 0) {
        GLint logLen = 0;
        glGetProgramiv(program.id(), GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::size_t>(logLen), '\0');
        glGetProgramInfoLog(program.id(), logLen, nullptr, log.data());
        qCCritical(Engine) << "Shader link error:" << log.c_str();
        return false; // program destructor cleans up
    }

    d->m_program = std::move(program);
    qCDebug(Engine) << "Shader program linked, id =" << d->m_program.id();
    return true;
}

bool Shader::isValid() const
{
    return static_cast<bool>(d->m_program);
}

void Shader::bind() const
{
    glUseProgram(d->m_program.id());
}

// ── setUniform specializations ────────────────────────────────────────────────

template<>
void Shader::setUniform(std::string_view name, const float &value)
{
    glUniform1f(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), value);
}

template<>
void Shader::setUniform(std::string_view name, const int &value)
{
    glUniform1i(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), value);
}

template<>
void Shader::setUniform(std::string_view name, const glm::vec2 &value)
{
    glUniform2fv(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), 1, glm::value_ptr(value));
}

template<>
void Shader::setUniform(std::string_view name, const glm::vec3 &value)
{
    glUniform3fv(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), 1, glm::value_ptr(value));
}

template<>
void Shader::setUniform(std::string_view name, const glm::vec4 &value)
{
    glUniform4fv(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), 1, glm::value_ptr(value));
}

template<>
void Shader::setUniform(std::string_view name, const glm::mat4 &value)
{
    glUniformMatrix4fv(glGetUniformLocation(d->m_program.id(), std::string{name}.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace engine
