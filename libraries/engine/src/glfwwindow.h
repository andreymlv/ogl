#pragma once

#include "engine_export.h"
#include "window.h"

#include <memory>
#include <string_view>

namespace engine
{

class GlfwContextPrivate;

// RAII wrapper for glfwInit / glfwTerminate.
// Call init() after construction and check isValid() before use.
// Construct exactly one before creating any GlfwWindow; destroyed last.
class ENGINE_EXPORT GlfwContext
{
public:
    GlfwContext();
    ~GlfwContext();

    GlfwContext(const GlfwContext &) = delete;
    GlfwContext &operator=(const GlfwContext &) = delete;

    bool init();
    bool isValid() const;

private:
    std::unique_ptr<GlfwContextPrivate> d;
};

class GlfwWindowPrivate;

// GLFW implementation of the Window interface.
// Call init() after construction and check isValid() before use.
// To swap to X11 or Win32: implement Window, change which type Application creates.
class ENGINE_EXPORT GlfwWindow : public Window
{
public:
    GlfwWindow();
    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow &) = delete;
    GlfwWindow &operator=(const GlfwWindow &) = delete;

    bool init(int width, int height, std::string_view title);
    bool isValid() const;

    bool shouldClose() const override;
    void swapBuffers() override;
    void pollEvents() override;

    int width() const override;
    int height() const override;

    bool isKeyPressed(Key key) const override;

    bool isGamepadConnected(int slot = 0) const override;
    bool isGamepadButtonPressed(GamepadButton button, int slot = 0) const override;
    float gamepadAxis(GamepadAxis axis, int slot = 0) const override;
    int firstGamepadSlot() const override;

    void setResizeCallback(std::function<void(int, int)> callback) override;

    void *nativeHandle() const override;

private:
    std::unique_ptr<GlfwWindowPrivate> d;
};

} // namespace engine
