#pragma once

// Template method bodies for Entity.
// This file is included at the bottom of scene.h, AFTER Scene is fully defined.
// Do NOT include scene.h from here — it is already in scope at the include site.

namespace engine
{

inline Entity::Entity(entt::entity handle, Scene *scene)
    : m_handle(handle)
    , m_scene(scene)
{
}

template<typename T, typename... Args>
T &Entity::addComponent(Args &&...args)
{
    return m_scene->registry().emplace<T>(m_handle, std::forward<Args>(args)...);
}

template<typename T>
T &Entity::getComponent()
{
    return m_scene->registry().get<T>(m_handle);
}

template<typename T>
const T &Entity::getComponent() const
{
    return m_scene->registry().get<T>(m_handle);
}

template<typename T>
bool Entity::hasComponent() const
{
    return m_scene->registry().all_of<T>(m_handle);
}

template<typename T>
void Entity::removeComponent()
{
    m_scene->registry().remove<T>(m_handle);
}

inline bool Entity::isValid() const
{
    return m_scene != nullptr && m_scene->registry().valid(m_handle);
}

} // namespace engine
