#include "camera2d.h"

#include <glm/gtc/matrix_transform.hpp>

namespace engine
{

class Camera2DPrivate
{
public:
    glm::mat4 m_projection{1.F};
    glm::mat4 m_viewProjection{1.F};

    glm::vec2 m_position{0.F, 0.F};
    float m_rotation = 0.F;
    float m_zoom = 1.F;

    void recalculate()
    {
        glm::mat4 view = glm::translate(glm::mat4(1.F), {-m_position.x, -m_position.y, 0.F});
        view = glm::rotate(view, glm::radians(m_rotation), {0.F, 0.F, 1.F});
        view = glm::scale(view, {m_zoom, m_zoom, 1.F});
        m_viewProjection = m_projection * view;
    }
};

Camera2D::Camera2D(float left, float right, float bottom, float top)
    : d(std::make_unique<Camera2DPrivate>())
{
    d->m_projection = glm::ortho(left, right, bottom, top);
    d->recalculate();
}

Camera2D::~Camera2D() = default;

void Camera2D::setPosition(glm::vec2 position)
{
    d->m_position = position;
    d->recalculate();
}

void Camera2D::setRotation(float degrees)
{
    d->m_rotation = degrees;
    d->recalculate();
}

void Camera2D::setZoom(float zoom)
{
    d->m_zoom = zoom;
    d->recalculate();
}

glm::vec2 Camera2D::position() const
{
    return d->m_position;
}

float Camera2D::rotation() const
{
    return d->m_rotation;
}

float Camera2D::zoom() const
{
    return d->m_zoom;
}

glm::mat4 Camera2D::viewProjectionMatrix() const
{
    return d->m_viewProjection;
}

} // namespace engine
