//
// Pong — minimal multiplayer with sound
//
// Usage:
//   ./pong host          -> start as host (left paddle, binds :7755)
//   ./pong <host_ip>     -> start as client (right paddle, connects to host)
//

#include "ponglayer.h"

#include <application.h>
#include <glfwwindow.h>
#include <miniaudioengine.h>
#include <openglrenderer.h>

#include <QCoreApplication>

#include <memory>

// ── PongGame ──────────────────────────────────────────────────────────────────

class PongGame : public engine::Application
{
public:
    bool onInit(PongLayer::Role role, const QString &hostIp)
    {
        auto layer = std::make_unique<PongLayer>(window(), audioEngine(), role, hostIp);
        pushLayer(std::move(layer));
        return true;
    }
};

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication qtApp(argc, argv);

    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2) {
        qCritical("Usage:  pong host  |  pong <host_ip>");
        return 1;
    }

    const bool isHost = (args.at(1) == u"host");
    const QString hostIp = isHost ? QString{} : args.at(1);

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, isHost ? "Pong — Host (W/S)" : "Pong — Client (W/S)")) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();
    auto audio = std::make_unique<engine::MiniaudioEngine>();

    PongGame game;
    if (!game.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }
    if (!game.onInit(isHost ? PongLayer::Role::Host : PongLayer::Role::Client, hostIp)) {
        return 1;
    }

    return game.run();
}
