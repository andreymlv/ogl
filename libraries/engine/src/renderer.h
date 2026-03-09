#pragma once

namespace engine
{

class Window;

// Pure interface for a graphics API backend.
// No OpenGL, no Vulkan — just the contract the engine drives.
// Swap implementations without touching Application or any game code.
class Renderer
{
public:
    virtual ~Renderer() = default;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    // Called once after the window is ready. Returns false on failure.
    virtual bool init(Window &window) = 0;

    // Called at the start of each frame — clear buffers, begin render pass, etc.
    virtual void beginFrame() = 0;

    // Called at the end of each frame — flush draw calls, end render pass, etc.
    virtual void endFrame() = 0;

    // Start / finish the ImGui frame. Game layers call ImGui:: between these.
    virtual void beginImGui() = 0;
    virtual void endImGui() = 0;

    // Called when the window is resized.
    virtual void onResize(int width, int height) = 0;

protected:
    Renderer() = default;
};

} // namespace engine
