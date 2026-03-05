#include "application.h"

#include "audioengine.h"
#include "layer.h"
#include "layerstack.h"
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
    std::unique_ptr<AudioEngine> m_audio;
    QElapsedTimer m_frameTimer;
    LayerStack m_layerStack;
};

Application::Application(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ApplicationPrivate>())
{
}

Application::~Application() = default;

bool Application::init(std::unique_ptr<Window> window, std::unique_ptr<Renderer> renderer, std::unique_ptr<AudioEngine> audio)
{
    if (!window) {
        qCCritical(Engine) << "Window is null";
        return false;
    }

    if (!renderer) {
        qCCritical(Engine) << "Renderer is null";
        return false;
    }

    if (!audio) {
        qCCritical(Engine) << "AudioEngine is null";
        return false;
    }

    if (!renderer->init(*window)) {
        qCCritical(Engine) << "Renderer failed to initialize";
        return false;
    }

    audio->init(); // non-fatal — no audio device in CI is acceptable

    d->m_window = std::move(window);
    d->m_renderer = std::move(renderer);
    d->m_audio = std::move(audio);

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

AudioEngine &Application::audioEngine()
{
    return *d->m_audio;
}

void Application::pushLayer(std::unique_ptr<Layer> layer)
{
    d->m_layerStack.pushLayer(std::move(layer));
}

std::unique_ptr<Layer> Application::popLayer(Layer *layer)
{
    return d->m_layerStack.popLayer(layer);
}

void Application::onUpdate(float dt)
{
    d->m_layerStack.update(dt);
}

void Application::onRender()
{
    d->m_layerStack.render();
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
