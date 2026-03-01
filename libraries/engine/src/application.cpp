#include "application.h"

#include "renderer.h"
#include "window.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QTimer>

#include "engine_logging.h"

namespace engine
{

class ApplicationPrivate
{
public:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    QElapsedTimer m_frameTimer;
};

Application::Application(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ApplicationPrivate>())
{
}

Application::~Application() = default;

bool Application::init(std::unique_ptr<Window> window, std::unique_ptr<Renderer> renderer)
{
    if (!window) {
        qCCritical(Engine) << "Window is null";
        return false;
    }

    if (!renderer) {
        qCCritical(Engine) << "Renderer is null";
        return false;
    }

    if (!renderer->init(*window)) {
        qCCritical(Engine) << "Renderer failed to initialize";
        return false;
    }

    d->m_window = std::move(window);
    d->m_renderer = std::move(renderer);

    // Wire resize events: window notifies renderer, renderer decides how to react.
    // GlfwWindow knows nothing about glViewport; VulkanRenderer would recreate its swapchain.
    d->m_window->setResizeCallback([this](int width, int height) {
        d->m_renderer->onResize(width, height);
    });

    return true;
}

int Application::run()
{
    d->m_frameTimer.start();

    QTimer loop;
    QObject::connect(&loop, &QTimer::timeout, this, [this] {
        tick();
    });
    loop.start(0);

    return QCoreApplication::exec();
}

Window &Application::window()
{
    return *d->m_window;
}

const Window &Application::window() const
{
    return *d->m_window;
}

Renderer &Application::renderer()
{
    return *d->m_renderer;
}

void Application::onUpdate(float dt)
{
    Q_UNUSED(dt)
}

void Application::onRender()
{
}

void Application::tick()
{
    if (d->m_window->shouldClose()) {
        QCoreApplication::quit();
        return;
    }

    d->m_window->pollEvents();

    const float dt = static_cast<float>(d->m_frameTimer.elapsed()) / 1000.0F;
    d->m_frameTimer.restart();

    onUpdate(dt);

    d->m_renderer->beginFrame();
    onRender();
    d->m_renderer->endFrame();

    d->m_window->swapBuffers();
}

} // namespace engine
