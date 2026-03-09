#pragma once

#include <functional>

namespace engine
{

// Platform-agnostic key identifiers.
// Values intentionally match GLFW_KEY_* so no translation table is needed
// in GlfwWindow, while keeping GLFW out of this header.
enum class Key {
    A = 65,
    D = 68,
    Space = 32,
    W = 87,
    S = 83,
    Left = 263,
    Right = 262,
    Up = 265,
    Down = 264,
    Escape = 256,
};

// Platform-agnostic gamepad button identifiers.
// Values match GLFW_GAMEPAD_BUTTON_* for zero-cost translation in GlfwWindow.
enum class GamepadButton {
    A = 0,
    B = 1,
    X = 2,
    Y = 3,
    LeftBumper = 4,
    RightBumper = 5,
    Back = 6,
    Start = 7,
    Guide = 8,
    LeftThumb = 9,
    RightThumb = 10,
    DpadUp = 11,
    DpadRight = 12,
    DpadDown = 13,
    DpadLeft = 14,
};

// Platform-agnostic gamepad axis identifiers.
// Values match GLFW_GAMEPAD_AXIS_*. Sticks range [-1,1], triggers [-1,1]
// where -1 = released, +1 = fully pressed. Stick Y: negative = up.
enum class GamepadAxis {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
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

    // Returns true if a gamepad with a standard mapping is connected at the given slot (0-15).
    virtual bool isGamepadConnected(int slot = 0) const = 0;

    // Returns true while the given gamepad button is held down.
    virtual bool isGamepadButtonPressed(GamepadButton button, int slot = 0) const = 0;

    // Returns the current value of a gamepad axis.
    // Sticks: [-1, 1]. Triggers: -1 = released, +1 = fully pressed.
    virtual float gamepadAxis(GamepadAxis axis, int slot = 0) const = 0;

    // Scans all joystick slots (0-15) and returns the index of the first slot
    // that has a gamepad mapping, or -1 if no mapped gamepad is found.
    virtual int firstGamepadSlot() const = 0;

    // Register a callback invoked whenever the framebuffer is resized.
    // The renderer uses this to react (glViewport, Vulkan swapchain recreation, etc.)
    // without the window knowing which graphics API is in use.
    virtual void setResizeCallback(std::function<void(int, int)> callback) = 0;

    // Returns the platform-specific native window handle (e.g. GLFWwindow*).
    // Used by subsystems that need the raw handle (e.g. ImGui backend).
    virtual void *nativeHandle() const = 0;

    // Not copyable — windows are unique resources.
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

protected:
    Window() = default;
};

} // namespace engine
