#pragma once

#include <functional>

namespace engine
{

// Platform-agnostic key identifiers.
// Values intentionally match GLFW_KEY_* so no translation table is needed
// in GlfwWindow, while keeping GLFW out of this header.
enum class Key {
    A = 65,
    Space = 32,
    W = 87,
    S = 83,
    Up = 265,
    Down = 264,
    Escape = 256,
};

// Pure interface. No GLFW, no X11, no Win32 — just the contract.
// Swap implementations without touching any code that uses Window.
class Window
{
public:
    virtual ~Window() = default;

    virtual bool shouldClose() const = 0;
    virtual void swapBuffers() = 0;
    virtual void pollEvents() = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;

    // Returns true while the given key is held down.
    virtual bool isKeyPressed(Key key) const = 0;

    // Register a callback invoked whenever the framebuffer is resized.
    // The renderer uses this to react (glViewport, Vulkan swapchain recreation, etc.)
    // without the window knowing which graphics API is in use.
    virtual void setResizeCallback(std::function<void(int, int)> callback) = 0;

    // Not copyable — windows are unique resources.
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

protected:
    Window() = default;
};

} // namespace engine
