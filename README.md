# OGL — 2D Game Engine

A 2D game engine built with modern C++20, OpenGL 3.3, Qt 6, and EnTT.
Follows KDE Frameworks coding conventions throughout.

Ships with two complete games — **Pong** (with LAN multiplayer and AI) and
**Flappy Bird** (with a PPO-trained neural-network AI).

---

## Features

- **Batched 2D renderer** — up to 1000 quads per draw call, 16 texture slots
- **ECS** — EnTT registry wrapped in a `Scene`/`Entity` API with built-in physics and render systems
- **Layer stack** — composable game layers with `onAttach`/`onDetach`/`onUpdate`/`onRender`
- **Audio** — miniaudio backend with procedural and file-based sound support
- **Dependency injection** — swap windowing (GLFW -> X11) or graphics (OpenGL -> Vulkan) without touching game code
- **Structured logging** — Qt logging categories, enable with `QT_LOGGING_RULES="engine.debug=true"`
- **Autotests** — 8 test suites via Qt Test / ECM
- **CI/CD** — GitHub Actions: Linux (glibc + musl static), Windows; nightly + tagged releases

---

## Tech Stack

| Concern | Library |
|---|---|
| Windowing / input | GLFW (via `GlfwWindow`) |
| OpenGL loader | glad |
| Math | GLM |
| ECS | EnTT |
| Event loop, logging, testing | Qt 6 Core / Test |
| Audio | miniaudio |
| Image loading | stb_image |
| Build system | CMake 4 + Ninja + vcpkg |
| KDE tooling | ECM (compiler settings, sanitizers, export headers, logging, tests) |

---

## Games

### Pong

Classic Pong with three play modes.

```sh
./pong              # solo — player vs AI
./pong host         # host a LAN game (UDP)
./pong <ip>         # connect to a LAN host
```

- Predictive AI with reaction delays and imperfection offsets
- UDP multiplayer via `QUdpSocket` (host sends state, client sends input)
- Procedurally generated sine-wave sound effects (440 Hz hit, 220 Hz score)
- First to 7 points; ball speeds up on each paddle hit

### Flappy Bird

Full clone with sprite textures, animation, and scrolling ground.

```sh
./flappybird        # press Space to flap, press A to cycle AI modes
```

- Three AI modes (toggle with `A`): manual, heuristic, ML (PPO neural network)
- ML model: `Linear(5->128)->Tanh->Linear(128->128)->Tanh->Linear(128->2)`, trained ~20M timesteps with Stable-Baselines3; weights baked into a C++ header for zero-dependency inference
- 5 WAV sound effects, 3-frame wing animation, digit-sprite score display
- Training pipeline in `games/flappybird/training/` (Python)

---

## Repository Layout

```
ogl/
├── CMakeLists.txt                # root: finds deps, adds subdirs
├── CMakePresets.json              # build presets (Debug, Release, Sanitizer, Coverage, …)
├── vcpkg.json                    # vcpkg dependencies
├── .clang-format / .clang-tidy   # KDE code style
│
├── libraries/engine/             # shared library (libengine.so)
│   ├── src/
│   │   ├── application.h/cpp         # QObject game loop, DI for Window/Renderer/Audio
│   │   ├── window.h                  # pure Window interface
│   │   ├── glfwwindow.h/cpp          # GLFW implementation
│   │   ├── renderer.h                # pure Renderer interface
│   │   ├── openglrenderer.h/cpp      # OpenGL implementation
│   │   ├── renderer2d.h/cpp          # batched quad/sprite renderer
│   │   ├── shader.h/cpp              # OpenGL shader program
│   │   ├── texture.h/cpp             # OpenGL texture (stb_image)
│   │   ├── globject.h                # RAII template for GL objects
│   │   ├── camera2d.h/cpp            # 2D orthographic camera
│   │   ├── layer.h/cpp               # layer base class
│   │   ├── layerstack.h/cpp          # ordered layer container
│   │   ├── scene.h/cpp               # ECS scene (wraps entt::registry)
│   │   ├── entity.h / entity_impl.h  # lightweight entity handle + templates
│   │   ├── components.h              # plain data components (Transform, Sprite, …)
│   │   ├── audioengine.h             # pure AudioEngine interface
│   │   ├── audioclip.h               # pure AudioClip interface
│   │   └── miniaudioengine.h/cpp     # miniaudio implementation
│   └── autotests/                # 8 Qt Test suites
│
├── games/
│   ├── pong/                     # Pong (AI, LAN multiplayer, procedural audio)
│   └── flappybird/               # Flappy Bird (sprites, ML AI, sound effects)
│       └── training/             # Python PPO training pipeline
│
├── examples/                     # learnopengl.com step-by-step
│   ├── 01_window/                # Hello Window
│   ├── 02_triangle/              # Hello Triangle (Shader + VAO/VBO)
│   └── 03_texture/               # Textured quad (two textures mixed)
│
└── meta/                         # build infra (vcpkg triplets, presets)
```

---

## Architecture

### Dependency injection

`Application` receives `Window`, `Renderer`, and `AudioEngine` via `init()`.
It knows nothing about GLFW, OpenGL, or miniaudio.

```
main.cpp
  ├── QCoreApplication app(argc, argv)
  ├── GlfwContext ctx;  ctx.init()
  ├── auto window   = make_unique<GlfwWindow>();   window->init(800, 600, "Game")
  ├── auto renderer = make_unique<OpenGLRenderer>()
  ├── auto audio    = make_unique<MiniaudioEngine>()
  └── GameApp game;
      game.init(move(window), move(renderer), move(audio))
      game.pushLayer(make_unique<GameLayer>(...))
      game.run()
```

### Game loop

```
Application::run()
  └── QTimer(0ms) → tick() every idle cycle
        ├── Window::shouldClose()  → quit if true
        ├── Window::pollEvents()
        ├── onUpdate(dt)           → forwards to LayerStack
        ├── Renderer::beginFrame()
        ├── onRender()             → forwards to LayerStack
        ├── Renderer::endFrame()
        └── Window::swapBuffers()
```

### Layer stack

Games push `Layer` subclasses onto `Application`. Update and render
calls forward to all layers in order.

```cpp
class Layer {
public:
    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float dt) {}
    virtual void onRender() {}
};
```

### ECS (EnTT)

`Scene` wraps an `entt::registry` via pImpl. `Entity` is a lightweight
handle with template methods for adding/getting/removing components.

**Components** — plain data structs:

```cpp
struct Transform   { glm::vec2 position, scale; float rotation; };
struct RigidBody2D { glm::vec2 velocity; };
struct Sprite      { std::shared_ptr<Texture> texture; glm::vec4 color; int zOrder; };
struct Tag         { std::string name; };
```

**Built-in systems** (inside `Scene`):

- `onUpdate(dt)` — applies velocity to position for all entities with `Transform` + `RigidBody2D`
- `onRender(renderer, camera)` — collects sprites, sorts by `zOrder`, draws via `Renderer2D`

### Renderer2D

Batched quad/sprite renderer. One draw call per `endScene()`.

```cpp
renderer.beginScene(camera);
renderer.drawQuad(pos, size, color);
renderer.drawSprite(pos, size, texture, rotation);
renderer.endScene();  // uploads VBO, binds textures, glDrawElements
```

---

## Build

### Prerequisites

- CMake 4+, Ninja, clang
- vcpkg (`$VCPKG_ROOT` must be set)

### Configure + build

```sh
# Pick a preset
cmake --preset Sanitizer    # ASan + UBSan + LSan → build/sanitizers
cmake --preset Debug        # debug → build/debug
cmake --preset Release      # RelWithDebInfo → build/release
cmake --preset Coverage     # gcov → build/coverage

# Build everything (or a specific target)
cmake --build build/sanitizers
cmake --build build/sanitizers --target engine
cmake --build build/sanitizers --target pong
```

### Test

```sh
ctest --test-dir build/sanitizers
ctest --test-dir build/sanitizers -R applicationtest -V
```

### Coverage

```sh
cmake --preset Coverage
cmake --build build/coverage
ctest --preset Coverage
cmake --build build/coverage --target coverage   # HTML → build/coverage/coverage-report/
```

---

## Coding Conventions

KDE Frameworks Coding Style. Compiler enforces `-fno-exceptions`.

### Naming

| Element | Convention | Example |
|---|---|---|
| Class / struct | `PascalCase` | `GlfwWindow` |
| Member variable | `m_camelCase` | `m_valid` |
| Static member | `s_camelCase` | `s_instance` |
| Method / function | `camelCase` | `shouldClose()` |
| Namespace | `lowercase` | `engine` |
| File | `lowercase.h/.cpp` | `glfwwindow.h` |

### Key rules

- No logic in constructors — use `bool init()` + `bool isValid()`
- No exceptions — `-fno-exceptions` is enforced
- pImpl for all non-trivial classes: `Private` forward-declared at namespace scope, defined in `.cpp`
- `ENGINE_EXPORT` on every public engine class (hidden visibility by default)
- `#pragma once` in all headers
- Do NOT list `.h` files in `target_sources` — ECM handles headers
- Logging: `qCDebug(Engine)` / `qCWarning(Engine)` / `qCCritical(Engine)` — never bare `qDebug()`

### Logging

Engine logs use the `engine` Qt logging category. Debug messages are off by
default; enable them with:

```sh
QT_LOGGING_RULES="engine.debug=true" ./pong
```

---

## References

- <https://learnopengl.com>
- <https://github.com/stevinz/awesome-game-engine-dev>
- <https://github.com/samuelcust/flappy-bird-assets> (Flappy Bird sprites)
