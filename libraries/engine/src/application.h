#pragma once

#include <engine_export.h>
#include "layerstack.h"

#include <QObject>

#include <memory>

namespace engine
{

class ApplicationPrivate;
class AudioEngine;
class Layer;
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

    // Takes ownership of the window, renderer and audio engine.
    // Returns false if any argument is null or if the renderer fails to initialize.
    // Audio engine init failure is non-fatal (no audio device in CI) — init() still returns true.
    bool init(std::unique_ptr<Window> window, std::unique_ptr<Renderer> renderer, std::unique_ptr<AudioEngine> audio);

    // Starts the Qt event loop. Blocks until the window is closed.
    // Requires QCoreApplication to already exist.
    int run();

    void pushLayer(std::unique_ptr<Layer> layer);
    std::unique_ptr<Layer> popLayer(Layer *layer);

    Window &window();
    const Window &window() const;
    Renderer &renderer();
    AudioEngine &audioEngine();

protected:
    virtual void onUpdate(float dt);
    virtual void onRender();

private:
    void tick();

    std::unique_ptr<ApplicationPrivate> d;
};

} // namespace engine
