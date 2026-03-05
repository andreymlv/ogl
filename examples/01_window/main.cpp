//
// learnopengl.com — Getting Started / Hello Window
// https://learnopengl.com/Getting-started/Hello-Window
//
// Opens an 800×600 window and clears it to a dark teal colour each frame.
// Demonstrates the minimal engine setup: GlfwContext + GlfwWindow +
// OpenGLRenderer injected into Application.
//

#include <application.h>
#include <miniaudioengine.h>
#include <glfwwindow.h>
#include <openglrenderer.h>

#include <QCoreApplication>

// ── WindowApp ─────────────────────────────────────────────────────────────────

// No overrides needed — OpenGLRenderer::beginFrame() already clears the
// colour buffer, and Application drives the loop automatically.
class WindowApp : public engine::Application
{
};

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication qtApp(argc, argv);

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, "Hello Window")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();

    WindowApp app;
    auto audio = std::make_unique<engine::MiniaudioEngine>();
    if (!app.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }

    return app.run();
}
