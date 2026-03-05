//
// Pong — solo, host/client multiplayer with sound
//
// Usage:
//   ./pong               -> solo mode (player vs AI)
//   ./pong solo          -> solo mode (player vs AI)
//   ./pong host          -> start as host (AI demo until client connects)
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

    PongLayer::Role role = PongLayer::Role::Solo;
    QString hostIp;
    if (args.size() >= 2) {
        if (args.at(1) == u"host") {
            role = PongLayer::Role::Host;
        } else if (args.at(1) != u"solo") {
            role = PongLayer::Role::Client;
            hostIp = args.at(1);
        }
    }

    engine::GlfwContext glfwCtx;
    if (!glfwCtx.init()) {
        return 1;
    }

    const char *title = "Pong — Solo (W/S)";
    if (role == PongLayer::Role::Host) {
        title = "Pong — Host (W/S)";
    } else if (role == PongLayer::Role::Client) {
        title = "Pong — Client (W/S)";
    }

    auto window = std::make_unique<engine::GlfwWindow>();
    if (!window->init(800, 600, title)) {
        return 1;
    }

    auto renderer = std::make_unique<engine::OpenGLRenderer>();
    auto audio = std::make_unique<engine::MiniaudioEngine>();

    PongGame game;
    if (!game.init(std::move(window), std::move(renderer), std::move(audio))) {
        return 1;
    }
    if (!game.onInit(role, hostIp)) {
        return 1;
    }

    return game.run();
}
