#pragma once

#include "engine_export.h"
#include "entity.h"

#include <entt/entt.hpp>

#include <memory>
#include <string>

namespace engine
{

class Camera2D;
class Renderer2D;
class ScenePrivate;

// Owns an entt::registry (via pImpl) and provides a high-level API for
// creating entities and running built-in systems each frame.
class ENGINE_EXPORT Scene
{
public:
    Scene();
    ~Scene();

    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;

    // Entity lifecycle
    Entity createEntity(const std::string &name = {});
    void destroyEntity(Entity entity);

    // Per-frame systems
    void onUpdate(float dt);
    void onRender(Renderer2D &renderer, const Camera2D &camera);

    // Registry access — needed by Entity template methods and custom systems.
    [[nodiscard]] entt::registry &registry();
    [[nodiscard]] const entt::registry &registry() const;

private:
    std::unique_ptr<ScenePrivate> d;
};

} // namespace engine

// Template bodies for Entity — included here, after Scene is fully defined,
// to break the circular entity.h ↔ scene.h dependency.
#include "entity_impl.h"
