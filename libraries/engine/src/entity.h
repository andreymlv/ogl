#pragma once

#include "engine_export.h"

#include <entt/entt.hpp>

namespace engine
{

class Scene;

// Thin non-owning value-type handle to an entt entity inside a Scene.
// Lifetime is managed by the Scene that created it.
// Template method bodies live in entity_impl.h, included at the bottom of scene.h
// once Scene is fully defined — do NOT include entity_impl.h from here.
class ENGINE_EXPORT Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene *scene);

    template<typename T, typename... Args>
    T &addComponent(Args &&...args);

    template<typename T>
    [[nodiscard]] T &getComponent();

    template<typename T>
    [[nodiscard]] const T &getComponent() const;

    template<typename T>
    [[nodiscard]] bool hasComponent() const;

    template<typename T>
    void removeComponent();

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] entt::entity handle() const { return m_handle; }

    bool operator==(const Entity &) const = default;

private:
    entt::entity m_handle{entt::null};
    Scene *m_scene = nullptr;
};

} // namespace engine
