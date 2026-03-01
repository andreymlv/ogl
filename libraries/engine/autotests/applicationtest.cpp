#include <QCoreApplication>
#include <QTest>
#include <QTimer>

#include "application.h"
#include "renderer.h"
#include "window.h"

namespace
{

// ── Mocks ─────────────────────────────────────────────────────────────────────

class MockWindow : public engine::Window
{
public:
    bool m_shouldClose = false;
    int m_pollCount = 0;
    int m_swapCount = 0;

    bool shouldClose() const override
    {
        return m_shouldClose;
    }
    void pollEvents() override
    {
        ++m_pollCount;
    }
    void swapBuffers() override
    {
        ++m_swapCount;
    }
    int width() const override
    {
        return 800;
    }
    int height() const override
    {
        return 600;
    }

    bool isKeyPressed(engine::Key /*key*/) const override
    {
        return false;
    }

    void setResizeCallback(std::function<void(int, int)> callback) override
    {
        m_resizeCallback = std::move(callback);
    }

    std::function<void(int, int)> m_resizeCallback;
};

class MockRenderer : public engine::Renderer
{
public:
    bool m_initResult = true;
    int m_beginCount = 0;
    int m_endCount = 0;

    bool init(engine::Window &) override
    {
        return m_initResult;
    }
    void beginFrame() override
    {
        ++m_beginCount;
    }
    void endFrame() override
    {
        ++m_endCount;
    }
    void onResize(int, int) override
    {
    }
};

// ── Helpers ───────────────────────────────────────────────────────────────────

std::unique_ptr<MockWindow> makeWindow()
{
    return std::make_unique<MockWindow>();
}

std::unique_ptr<MockRenderer> makeRenderer()
{
    return std::make_unique<MockRenderer>();
}

} // namespace

// ── Test class ────────────────────────────────────────────────────────────────

class ApplicationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // init() — null checks
    void initWithNullWindowReturnsFalse();
    void initWithNullRendererReturnsFalse();

    // init() — renderer failure
    void initWhenRendererInitFailsReturnsFalse();

    // init() — success
    void initWithValidInputReturnsTrue();

    // accessors after successful init
    void windowReturnsInjectedWindow();
    void rendererReturnsInjectedRenderer();

    // run() — window signals close immediately
    void runExitsWhenWindowShouldClose();

    // run() — one normal frame
    void runTickCallsRendererBeginAndEnd();
    void runTickCallsPollAndSwap();
    void runTickCallsOnUpdate();
    void runTickCallsOnRender();
};

// ── init() tests ──────────────────────────────────────────────────────────────

void ApplicationTest::initWithNullWindowReturnsFalse()
{
    engine::Application app;
    QVERIFY(!app.init(nullptr, makeRenderer()));
}

void ApplicationTest::initWithNullRendererReturnsFalse()
{
    engine::Application app;
    QVERIFY(!app.init(makeWindow(), nullptr));
}

void ApplicationTest::initWhenRendererInitFailsReturnsFalse()
{
    auto renderer = makeRenderer();
    renderer->m_initResult = false;

    engine::Application app;
    QVERIFY(!app.init(makeWindow(), std::move(renderer)));
}

void ApplicationTest::initWithValidInputReturnsTrue()
{
    engine::Application app;
    QVERIFY(app.init(makeWindow(), makeRenderer()));
}

// ── accessor tests ────────────────────────────────────────────────────────────

void ApplicationTest::windowReturnsInjectedWindow()
{
    auto *rawWindow = new MockWindow;
    engine::Application app;
    app.init(std::unique_ptr<MockWindow>(rawWindow), makeRenderer());

    QCOMPARE(&app.window(), rawWindow);
}

void ApplicationTest::rendererReturnsInjectedRenderer()
{
    auto *rawRenderer = new MockRenderer;
    engine::Application app;
    app.init(makeWindow(), std::unique_ptr<MockRenderer>(rawRenderer));

    QCOMPARE(&app.renderer(), rawRenderer);
}

// ── run() tests ───────────────────────────────────────────────────────────────

// Subclass that exposes tick counters for run() tests.
class TestApplication : public engine::Application
{
public:
    int m_updateCount = 0;
    int m_renderCount = 0;
    float m_lastDt = 0.0F;

protected:
    void onUpdate(float dt) override
    {
        m_lastDt = dt;
        ++m_updateCount;
    }

    void onRender() override
    {
        ++m_renderCount;
    }
};

void ApplicationTest::runExitsWhenWindowShouldClose()
{
    auto window = makeWindow();
    window->m_shouldClose = true;

    TestApplication app;
    app.init(std::move(window), makeRenderer());

    // shouldClose is already true — run() exits on the very first tick.
    const int exitCode = app.run();
    QCOMPARE(exitCode, 0);
}

void ApplicationTest::runTickCallsRendererBeginAndEnd()
{
    auto window = makeWindow();
    auto *rawRenderer = new MockRenderer;

    // Close after first render frame so run() exits.
    auto *rawWindow = window.get();

    TestApplication app;
    app.init(std::move(window), std::unique_ptr<MockRenderer>(rawRenderer));

    // Delay slightly so at least one tick runs before we signal close.
    QTimer::singleShot(10, &app, [rawWindow] {
        rawWindow->m_shouldClose = true;
    });

    app.run();

    QVERIFY(rawRenderer->m_beginCount >= 1);
    QVERIFY(rawRenderer->m_endCount >= 1);
    QCOMPARE(rawRenderer->m_beginCount, rawRenderer->m_endCount);
}

void ApplicationTest::runTickCallsPollAndSwap()
{
    auto window = makeWindow();
    auto *rawWindow = window.get();

    TestApplication app;
    app.init(std::move(window), makeRenderer());

    // Delay slightly so at least one tick runs before we signal close.
    QTimer::singleShot(10, &app, [rawWindow] {
        rawWindow->m_shouldClose = true;
    });

    app.run();

    QVERIFY(rawWindow->m_pollCount >= 1);
    QVERIFY(rawWindow->m_swapCount >= 1);
}

void ApplicationTest::runTickCallsOnUpdate()
{
    auto window = makeWindow();
    auto *rawWindow = window.get();

    TestApplication app;
    app.init(std::move(window), makeRenderer());

    // Delay slightly so at least one tick runs before we signal close.
    QTimer::singleShot(10, &app, [rawWindow] {
        rawWindow->m_shouldClose = true;
    });

    app.run();

    QVERIFY(app.m_updateCount >= 1);
}

void ApplicationTest::runTickCallsOnRender()
{
    auto window = makeWindow();
    auto *rawWindow = window.get();

    TestApplication app;
    app.init(std::move(window), makeRenderer());

    // Delay slightly so at least one tick runs before we signal close.
    QTimer::singleShot(10, &app, [rawWindow] {
        rawWindow->m_shouldClose = true;
    });

    app.run();

    QVERIFY(app.m_renderCount >= 1);
}

QTEST_GUILESS_MAIN(ApplicationTest)
#include "applicationtest.moc"
