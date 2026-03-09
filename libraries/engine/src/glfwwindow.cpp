#include "glfwwindow.h"

#include <GLFW/glfw3.h>

#include <QLoggingCategory>

#include "engine_logging.h"

namespace engine
{

// ── GlfwContext ───────────────────────────────────────────────────────────────

class GlfwContextPrivate
{
public:
    bool m_valid = false;
};

GlfwContext::GlfwContext()
    : d(std::make_unique<GlfwContextPrivate>())
{
}

GlfwContext::~GlfwContext()
{
    if (d->m_valid) {
        qCDebug(Engine) << "Terminating GLFW";
        glfwTerminate();
    }
}

bool GlfwContext::init()
{
    d->m_valid = glfwInit() != 0;
    if (!d->m_valid) {
        qCCritical(Engine) << "Failed to initialize GLFW";
        return false;
    }

    qCDebug(Engine) << "GLFW initialized";

    // Add SDL gamepad mappings for controllers not in GLFW's built-in database.
    // Format: SDL_GameControllerDB (https://github.com/gabomdq/SDL_GameControllerDB).
    // Each string is null-terminated; glfwUpdateGamepadMappings takes one at a time.
    static constexpr const char *kExtraMappings[] = {
        // 8BitDo Ultimate 2 Wireless Controller (USB, Vendor 2dc8, Product 310b)
        // GUID obtained from glfwGetJoystickGUID(); axis/button layout matches Xbox 360.
        "03000000c82d00000b31000014010000,8BitDo Ultimate 2 Wireless Controller,"
        "a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,"
        "guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,"
        "rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,"
        "start:b7,x:b2,y:b3,platform:Linux,",
    };

    for (const char *mapping : kExtraMappings) {
        if (glfwUpdateGamepadMappings(mapping) == GLFW_FALSE) {
            qCWarning(Engine) << "Failed to add gamepad mapping:" << mapping;
        } else {
            qCDebug(Engine) << "Added custom gamepad mapping";
        }
    }

    return true;
}

bool GlfwContext::isValid() const
{
    return d->m_valid;
}

// ── GlfwWindow ────────────────────────────────────────────────────────────────

class GlfwWindowPrivate
{
public:
    struct Deleter {
        void operator()(GLFWwindow *w) const noexcept
        {
            glfwDestroyWindow(w);
        }
    };

    std::unique_ptr<GLFWwindow, Deleter> m_handle;
    std::function<void(int, int)> m_resizeCallback;
    int m_width = 0;
    int m_height = 0;
};

GlfwWindow::GlfwWindow()
    : d(std::make_unique<GlfwWindowPrivate>())
{
}

GlfwWindow::~GlfwWindow()
{
    if (d->m_handle) {
        qCDebug(Engine) << "Destroying GLFW window";
    }
}

bool GlfwWindow::init(int width, int height, std::string_view title)
{
    d->m_width = width;
    d->m_height = height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    d->m_handle.reset(glfwCreateWindow(width, height, title.data(), nullptr, nullptr));
    if (!d->m_handle) {
        qCCritical(Engine) << "Failed to create GLFW window";
        return false;
    }

    glfwMakeContextCurrent(d->m_handle.get());
    glfwSwapInterval(0);
    glfwSetWindowUserPointer(d->m_handle.get(), d.get());

    glfwSetFramebufferSizeCallback(d->m_handle.get(), [](GLFWwindow *w, int newWidth, int newHeight) {
        auto *priv = static_cast<GlfwWindowPrivate *>(glfwGetWindowUserPointer(w));
        priv->m_width = newWidth;
        priv->m_height = newHeight;
        if (priv->m_resizeCallback) {
            priv->m_resizeCallback(newWidth, newHeight);
        }
    });

    qCDebug(Engine) << "GLFW window created:" << width << "x" << height;
    return true;
}

bool GlfwWindow::isValid() const
{
    return d->m_handle != nullptr;
}

bool GlfwWindow::shouldClose() const
{
    return glfwWindowShouldClose(d->m_handle.get()) != 0;
}

void GlfwWindow::swapBuffers()
{
    glfwSwapBuffers(d->m_handle.get());
}

void GlfwWindow::pollEvents()
{
    glfwPollEvents();
}

int GlfwWindow::width() const
{
    return d->m_width;
}

int GlfwWindow::height() const
{
    return d->m_height;
}

bool GlfwWindow::isKeyPressed(Key key) const
{
    return glfwGetKey(d->m_handle.get(), static_cast<int>(key)) == GLFW_PRESS;
}

bool GlfwWindow::isGamepadConnected(int slot) const
{
    return glfwJoystickIsGamepad(slot) == GLFW_TRUE;
}

bool GlfwWindow::isGamepadButtonPressed(GamepadButton button, int slot) const
{
    GLFWgamepadstate state;
    if (glfwGetGamepadState(slot, &state) == 0) {
        return false;
    }
    return state.buttons[static_cast<int>(button)] == GLFW_PRESS;
}

float GlfwWindow::gamepadAxis(GamepadAxis axis, int slot) const
{
    GLFWgamepadstate state;
    if (glfwGetGamepadState(slot, &state) == 0) {
        return 0.0F;
    }
    return state.axes[static_cast<int>(axis)];
}

void GlfwWindow::setResizeCallback(std::function<void(int, int)> callback)
{
    d->m_resizeCallback = std::move(callback);
}

int GlfwWindow::firstGamepadSlot() const
{
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; ++i) {
        if (glfwJoystickIsGamepad(i) == GLFW_TRUE) {
            return i;
        }
    }
    return -1;
}

void *GlfwWindow::nativeHandle() const
{
    return d->m_handle.get();
}

} // namespace engine
