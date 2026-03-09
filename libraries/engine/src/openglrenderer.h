#pragma once

#include "engine_export.h"
#include "renderer.h"

#include <memory>

namespace engine
{

class OpenGLRendererPrivate;
class Window;

// OpenGL implementation of the Renderer interface.
// Call init(window) after construction and check isValid() before use.
// To switch to Vulkan: implement Renderer, inject VulkanRenderer instead.
class ENGINE_EXPORT OpenGLRenderer : public Renderer
{
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer &) = delete;
    OpenGLRenderer &operator=(const OpenGLRenderer &) = delete;

    bool init(Window &window) override;
    bool isValid() const;

    void beginFrame() override;
    void endFrame() override;
    void beginImGui() override;
    void endImGui() override;
    void onResize(int width, int height) override;

private:
    std::unique_ptr<OpenGLRendererPrivate> d;
};

} // namespace engine
