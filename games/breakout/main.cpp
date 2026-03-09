#include "breakoutlayer.h"

#include <application.h>
#include <glfwwindow.h>
#include <miniaudioengine.h>
#include <openglrenderer.h>

#include <QCoreApplication>

#include <memory>

// ── BreakoutGame ────────────────────────────────────────────────────────────

class BreakoutGame : public engine::Application
{
public:
    bool onInit()
    {
        auto layer = std::make_unique<BreakoutLayer>(window(), audioEngine());
        pushLayer(std::move(layer));
        return true;
    }
};

// ── main ────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication qtApp(argc, argv);
    qtApp.setOrganizationName(QStringLiteral("Breakout"));
    qtApp.setApplicationName(QStringLiteral("Breakout"));

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, "Breakout")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();
    auto audio = std::make_unique<engine::MiniaudioEngine>();

    BreakoutGame game;
    if (!game.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }
    if (!game.onInit()) {
        return 1;
    }

    return game.run();
}
