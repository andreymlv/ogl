#include "scene.h"

#include "camera2d.h"
#include "components.h"
#include "renderer2d.h"

#include <algorithm>
#include <vector>

namespace engine
{

// ── Private ───────────────────────────────────────────────────────────────────

class ScenePrivate
{
public:
    entt::registry m_registry;
};

// ── Scene ─────────────────────────────────────────────────────────────────────

Scene::Scene()
    : d(std::make_unique<ScenePrivate>())
{
}

Scene::~Scene() = default;

Entity Scene::createEntity(const std::string &name)
{
    Entity entity{d->m_registry.create(), this};
    entity.addComponent<Tag>(name);
    return entity;
}

void Scene::destroyEntity(Entity entity)
{
    d->m_registry.destroy(entity.handle());
}

void Scene::onUpdate(float dt)
{
    auto view = d->m_registry.view<Transform, RigidBody2D>();
    for (auto e : view) {
        auto &tf = view.get<Transform>(e);
        const auto &rb = view.get<RigidBody2D>(e);
        tf.position += rb.velocity * dt;
    }
}

void Scene::onRender(Renderer2D &renderer, const Camera2D &camera)
{
    renderer.beginScene(camera);

    struct DrawCall {
        glm::vec2 position;
        glm::vec2 scale;
        glm::vec4 color;
        std::shared_ptr<Texture> texture;
        float rotation;
        int zOrder;
    };

    std::vector<DrawCall> calls;
    auto view = d->m_registry.view<Transform, Sprite>();
    for (auto e : view) {
        const auto &tf = view.get<Transform>(e);
        const auto &sp = view.get<Sprite>(e);
        calls.push_back({tf.position, tf.scale, sp.color, sp.texture, tf.rotation, sp.zOrder});
    }

    std::ranges::sort(calls, [](const DrawCall &a, const DrawCall &b) {
        return a.zOrder < b.zOrder;
    });

    for (const auto &call : calls) {
        if (call.texture) {
            renderer.drawSprite(call.position, call.scale, *call.texture, call.rotation);
        } else {
            renderer.drawQuad(call.position, call.scale, call.color);
        }
    }

    renderer.endScene();
}

entt::registry &Scene::registry()
{
    return d->m_registry;
}

const entt::registry &Scene::registry() const
{
    return d->m_registry;
}

} // namespace engine
