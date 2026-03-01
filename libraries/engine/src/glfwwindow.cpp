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
        glfwTerminate();
    }
}

bool GlfwContext::init()
{
    d->m_valid = glfwInit() != 0;
    if (!d->m_valid) {
        qCCritical(Engine) << "Failed to initialize GLFW";
    }
    return d->m_valid;
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

GlfwWindow::~GlfwWindow() = default;

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
    glfwSetWindowUserPointer(d->m_handle.get(), d.get());

    glfwSetFramebufferSizeCallback(d->m_handle.get(), [](GLFWwindow *w, int newWidth, int newHeight) {
        auto *priv = static_cast<GlfwWindowPrivate *>(glfwGetWindowUserPointer(w));
        priv->m_width = newWidth;
        priv->m_height = newHeight;
        if (priv->m_resizeCallback) {
            priv->m_resizeCallback(newWidth, newHeight);
        }
    });

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

void GlfwWindow::setResizeCallback(std::function<void(int, int)> callback)
{
    d->m_resizeCallback = std::move(callback);
}

} // namespace engine
