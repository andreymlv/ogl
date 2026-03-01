#pragma once

#include <engine_export.h>

#include <QObject>

#include <memory>

namespace engine
{

class ApplicationPrivate;
class Renderer;
class Window;

// Drives the game loop using the Qt event loop.
// Subclass and override onUpdate() / onRender() to implement a game.
//
// Usage:
//   QCoreApplication app(argc, argv);  // caller owns Qt lifecycle
//   myGame.init(std::move(window), std::move(renderer));
//   return myGame.run();
class ENGINE_EXPORT Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    // Takes ownership of the window and renderer.
    // Returns false if either is null or if the renderer fails to initialize.
    bool init(std::unique_ptr<Window> window, std::unique_ptr<Renderer> renderer);

    // Starts the Qt event loop. Blocks until the window is closed.
    // Requires QCoreApplication to already exist.
    int run();

    Window &window();
    const Window &window() const;
    Renderer &renderer();

protected:
    virtual void onUpdate(float dt);
    virtual void onRender();

private:
    void tick();

    std::unique_ptr<ApplicationPrivate> d;
};

} // namespace engine
