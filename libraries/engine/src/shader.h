#pragma once

#include "engine_export.h"

#include <memory>
#include <string_view>

namespace engine
{

class ShaderPrivate;

// Compiles and links a GLSL vertex + fragment shader pair.
// Call init() after construction; check isValid() before use.
// Requires an active OpenGL context (gladLoadGLLoader must have run).
class ENGINE_EXPORT Shader
{
public:
    Shader();
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    bool init(std::string_view vertSrc, std::string_view fragSrc);
    bool isValid() const;

    void bind() const;

    // Supported specializations (defined in shader.cpp):
    //   float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4
    template<typename T>
    void setUniform(std::string_view name, const T &value);

private:
    std::unique_ptr<ShaderPrivate> d;
};

} // namespace engine
