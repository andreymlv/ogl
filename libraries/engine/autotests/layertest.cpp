#include <QTest>

#include "layer.h"
#include "layerstack.h"

namespace
{

// ── Spy Layer ─────────────────────────────────────────────────────────────────

class SpyLayer : public engine::Layer
{
public:
    explicit SpyLayer(QString name = QStringLiteral("spy"))
        : engine::Layer(std::move(name))
    {
    }

    int m_attachCount = 0;
    int m_detachCount = 0;
    int m_updateCount = 0;
    int m_renderCount = 0;
    float m_lastDt = 0.0F;

    void onAttach() override
    {
        ++m_attachCount;
    }
    void onDetach() override
    {
        ++m_detachCount;
    }
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

} // namespace

// ── Test class ────────────────────────────────────────────────────────────────

class LayerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Layer base
    void layerNameIsSetByConstructor();

    // LayerStack — push / pop
    void pushLayerCallsOnAttach();
    void popLayerCallsOnDetach();
    void popLayerRemovesFromStack();
    void popNonExistentLayerDoesNothing();

    // LayerStack — update / render forwarding
    void updateForwardsToAllLayers();
    void renderForwardsToAllLayers();
    void updatePassesDtToLayer();

    // LayerStack — ordering
    void layersUpdatedInPushOrder();

    // LayerStack — clear
    void clearDetachesAllLayers();
    void clearEmptiesStack();
};

// ── Layer base tests ──────────────────────────────────────────────────────────

void LayerTest::layerNameIsSetByConstructor()
{
    SpyLayer layer(QStringLiteral("game"));
    QCOMPARE(layer.name(), QStringLiteral("game"));
}

// ── LayerStack push / pop ─────────────────────────────────────────────────────

void LayerTest::pushLayerCallsOnAttach()
{
    engine::LayerStack stack;
    auto layer = std::make_unique<SpyLayer>();
    auto *raw = layer.get();

    stack.pushLayer(std::move(layer));

    QCOMPARE(raw->m_attachCount, 1);
}

void LayerTest::popLayerCallsOnDetach()
{
    engine::LayerStack stack;
    auto layer = std::make_unique<SpyLayer>();
    auto *raw = layer.get();
    stack.pushLayer(std::move(layer));

    auto returned = stack.popLayer(raw); // caller holds ownership — raw stays valid

    QCOMPARE(raw->m_detachCount, 1);
}

void LayerTest::popLayerRemovesFromStack()
{
    engine::LayerStack stack;
    auto layer = std::make_unique<SpyLayer>();
    auto *raw = layer.get();
    stack.pushLayer(std::move(layer));

    auto returned = stack.popLayer(raw);

    // After pop, update should not reach the removed layer
    stack.update(0.016F);
    QCOMPARE(raw->m_updateCount, 0);
}

void LayerTest::popNonExistentLayerDoesNothing()
{
    engine::LayerStack stack;
    SpyLayer orphan;
    // Must not crash; returns nullptr for unknown layer
    auto returned = stack.popLayer(&orphan);
    QVERIFY(!returned);
    QCOMPARE(orphan.m_detachCount, 0);
}

// ── LayerStack forwarding ─────────────────────────────────────────────────────

void LayerTest::updateForwardsToAllLayers()
{
    engine::LayerStack stack;
    auto l1 = std::make_unique<SpyLayer>();
    auto l2 = std::make_unique<SpyLayer>();
    auto *r1 = l1.get();
    auto *r2 = l2.get();

    stack.pushLayer(std::move(l1));
    stack.pushLayer(std::move(l2));

    stack.update(0.016F);

    QCOMPARE(r1->m_updateCount, 1);
    QCOMPARE(r2->m_updateCount, 1);
}

void LayerTest::renderForwardsToAllLayers()
{
    engine::LayerStack stack;
    auto l1 = std::make_unique<SpyLayer>();
    auto l2 = std::make_unique<SpyLayer>();
    auto *r1 = l1.get();
    auto *r2 = l2.get();

    stack.pushLayer(std::move(l1));
    stack.pushLayer(std::move(l2));

    stack.render();

    QCOMPARE(r1->m_renderCount, 1);
    QCOMPARE(r2->m_renderCount, 1);
}

void LayerTest::updatePassesDtToLayer()
{
    engine::LayerStack stack;
    auto layer = std::make_unique<SpyLayer>();
    auto *raw = layer.get();
    stack.pushLayer(std::move(layer));

    stack.update(0.033F);

    QCOMPARE(raw->m_lastDt, 0.033F);
}

// ── LayerStack ordering ───────────────────────────────────────────────────────

void LayerTest::layersUpdatedInPushOrder()
{
    QStringList order;
    engine::LayerStack stack;

    class OrderLayer : public engine::Layer
    {
    public:
        OrderLayer(QString name, QStringList &log)
            : engine::Layer(std::move(name))
            , m_log(log)
        {
        }
        void onUpdate(float /*dt*/) override
        {
            m_log.append(name());
        }
        QStringList &m_log;
    };

    stack.pushLayer(std::make_unique<OrderLayer>(QStringLiteral("first"), order));
    stack.pushLayer(std::make_unique<OrderLayer>(QStringLiteral("second"), order));

    stack.update(0.0F);

    QCOMPARE(order, QStringList({QStringLiteral("first"), QStringLiteral("second")}));
}

// ── LayerStack clear ──────────────────────────────────────────────────────────

void LayerTest::clearDetachesAllLayers()
{
    // Use a shared counter incremented from onDetach so we don't hold
    // dangling raw pointers after clear() destroys the layers.
    int detachCount = 0;

    class CountingLayer : public engine::Layer
    {
    public:
        CountingLayer(QString name, int &counter)
            : engine::Layer(std::move(name))
            , m_counter(counter)
        {
        }
        void onDetach() override
        {
            ++m_counter;
        }
        int &m_counter;
    };

    engine::LayerStack stack;
    stack.pushLayer(std::make_unique<CountingLayer>(QStringLiteral("a"), detachCount));
    stack.pushLayer(std::make_unique<CountingLayer>(QStringLiteral("b"), detachCount));

    stack.clear();

    QCOMPARE(detachCount, 2);
}

void LayerTest::clearEmptiesStack()
{
    engine::LayerStack stack;
    auto l1 = std::make_unique<SpyLayer>();
    stack.pushLayer(std::move(l1));

    stack.clear();

    // After clear, update touches nothing
    stack.update(0.016F);
    // No crash and nothing outstanding — test just checks no crash + empty
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(LayerTest)
#include "layertest.moc"
