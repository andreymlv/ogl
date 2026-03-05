#pragma once

#include "engine_export.h"

#include <glm/glm.hpp>

#include <memory>

namespace engine
{

class Camera2D;
class Renderer2DPrivate;
class Texture;


// Batched 2D drawing API.  One draw call per frame regardless of quad count.
//
// Typical usage per frame:
//   renderer2d.beginScene();
//   renderer2d.drawQuad({0.f, 0.f}, {100.f, 100.f}, {1.f, 0.f, 0.f, 1.f});
//   renderer2d.endScene();   // flushes a single glDrawElements call
//
// Requires an active OpenGL context.  Call init() once after GLAD is loaded.
class ENGINE_EXPORT Renderer2D
{
public:
    Renderer2D();
    ~Renderer2D();

    Renderer2D(const Renderer2D &) = delete;
    Renderer2D &operator=(const Renderer2D &) = delete;

    bool init();
    bool isValid() const;

    void beginScene(const Camera2D &camera);
    void drawQuad(glm::vec2 position, glm::vec2 size, glm::vec4 color);
    void drawSprite(glm::vec2 position, glm::vec2 size, const Texture &texture);
    void endScene();

    int quadCount() const; // number of quads submitted since beginScene()

private:
    std::unique_ptr<Renderer2DPrivate> d;
};

} // namespace engine
