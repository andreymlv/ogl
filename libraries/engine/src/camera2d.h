#pragma once

#include "engine_export.h"

#include <glm/glm.hpp>

#include <memory>

namespace engine
{

class Camera2DPrivate;

// Orthographic 2D camera.
//
// Produces a view-projection matrix for use in Renderer2D::beginScene().
//
// View = scale(zoom) * rotate(rotation) * translate(-position)
// The ortho projection bounds are set once at construction.
class ENGINE_EXPORT Camera2D
{
public:
    Camera2D(float left, float right, float bottom, float top);
    ~Camera2D();

    Camera2D(const Camera2D &) = delete;
    Camera2D &operator=(const Camera2D &) = delete;

    void setPosition(glm::vec2 position);
    void setRotation(float degrees);
    void setZoom(float zoom);

    [[nodiscard]] glm::vec2 position() const;
    [[nodiscard]] float rotation() const;
    [[nodiscard]] float zoom() const;

    [[nodiscard]] glm::mat4 viewProjectionMatrix() const;

private:
    std::unique_ptr<Camera2DPrivate> d;
};

} // namespace engine
