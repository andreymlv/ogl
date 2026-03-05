#include <QTest>

#include "components.h"
#include "scene.h"

// ── Test class ────────────────────────────────────────────────────────────────

class SceneTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Entity creation
    void createEntityReturnsValidEntity();
    void createEntityWithNameSetsTagComponent();
    void createEntityWithEmptyNameSetsEmptyTag();
    void twoEntitiesAreNotEqual();

    // destroyEntity
    void destroyEntityMakesItInvalid();

    // addComponent / hasComponent / getComponent
    void addComponentAttachesComponent();
    void hasComponentReturnsFalseBeforeAdd();
    void hasComponentReturnsTrueAfterAdd();
    void getComponentReturnsReference();
    void addComponentWithArgs();

    // removeComponent
    void removeComponentDetachesComponent();

    // onUpdate — RigidBody2D integrates into Transform
    void onUpdateMovesTransformByVelocity();
    void onUpdateWithZeroDtDoesNotMove();
    void onUpdateEntityWithoutRigidBodyIsUnaffected();

    // onRender — quad count via mock (no GL; we just verify no crash + count via Scene::registry)
    void renderSystemIteratesSprites();
};

// ── helpers ───────────────────────────────────────────────────────────────────

static engine::Entity makeMovingEntity(engine::Scene &scene, glm::vec2 pos, glm::vec2 vel)
{
    auto e = scene.createEntity("mover");
    e.addComponent<engine::Transform>().position = pos;
    e.addComponent<engine::RigidBody2D>().velocity = vel;
    return e;
}

// ── Entity creation ───────────────────────────────────────────────────────────

void SceneTest::createEntityReturnsValidEntity()
{
    engine::Scene scene;
    auto e = scene.createEntity("hero");
    QVERIFY(e.isValid());
}

void SceneTest::createEntityWithNameSetsTagComponent()
{
    engine::Scene scene;
    auto e = scene.createEntity("hero");
    QVERIFY(e.hasComponent<engine::Tag>());
    QCOMPARE(e.getComponent<engine::Tag>().name, std::string("hero"));
}

void SceneTest::createEntityWithEmptyNameSetsEmptyTag()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    QVERIFY(e.hasComponent<engine::Tag>());
    QCOMPARE(e.getComponent<engine::Tag>().name, std::string());
}

void SceneTest::twoEntitiesAreNotEqual()
{
    engine::Scene scene;
    auto a = scene.createEntity("a");
    auto b = scene.createEntity("b");
    QVERIFY(a != b);
}

// ── destroyEntity ─────────────────────────────────────────────────────────────

void SceneTest::destroyEntityMakesItInvalid()
{
    engine::Scene scene;
    auto e = scene.createEntity("temp");
    QVERIFY(e.isValid());
    scene.destroyEntity(e);
    QVERIFY(!e.isValid());
}

// ── addComponent / hasComponent / getComponent ────────────────────────────────

void SceneTest::addComponentAttachesComponent()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    e.addComponent<engine::Transform>();
    QVERIFY(e.hasComponent<engine::Transform>());
}

void SceneTest::hasComponentReturnsFalseBeforeAdd()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    QVERIFY(!e.hasComponent<engine::Transform>());
}

void SceneTest::hasComponentReturnsTrueAfterAdd()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    e.addComponent<engine::Transform>();
    QVERIFY(e.hasComponent<engine::Transform>());
}

void SceneTest::getComponentReturnsReference()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    e.addComponent<engine::Transform>().position = {3.F, 7.F};
    const auto &tf = e.getComponent<engine::Transform>();
    QCOMPARE(tf.position, glm::vec2(3.F, 7.F));
}

void SceneTest::addComponentWithArgs()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    e.addComponent<engine::Tag>("player");
    // Tag was already added by createEntity; emplace replaces it.
    QCOMPARE(e.getComponent<engine::Tag>().name, std::string("player"));
}

// ── removeComponent ───────────────────────────────────────────────────────────

void SceneTest::removeComponentDetachesComponent()
{
    engine::Scene scene;
    auto e = scene.createEntity();
    e.addComponent<engine::Transform>();
    QVERIFY(e.hasComponent<engine::Transform>());
    e.removeComponent<engine::Transform>();
    QVERIFY(!e.hasComponent<engine::Transform>());
}

// ── onUpdate ──────────────────────────────────────────────────────────────────

void SceneTest::onUpdateMovesTransformByVelocity()
{
    engine::Scene scene;
    auto e = makeMovingEntity(scene, {0.F, 0.F}, {1.F, 2.F});

    scene.onUpdate(0.5F); // dt = 0.5s → pos = vel * dt = {0.5, 1.0}

    const auto &tf = e.getComponent<engine::Transform>();
    QCOMPARE(tf.position.x, 0.5F);
    QCOMPARE(tf.position.y, 1.0F);
}

void SceneTest::onUpdateWithZeroDtDoesNotMove()
{
    engine::Scene scene;
    auto e = makeMovingEntity(scene, {5.F, 3.F}, {10.F, 10.F});

    scene.onUpdate(0.F);

    const auto &tf = e.getComponent<engine::Transform>();
    QCOMPARE(tf.position, glm::vec2(5.F, 3.F));
}

void SceneTest::onUpdateEntityWithoutRigidBodyIsUnaffected()
{
    engine::Scene scene;
    auto e = scene.createEntity("static");
    e.addComponent<engine::Transform>().position = {1.F, 2.F};
    // No RigidBody2D — onUpdate must not crash and must not move it.

    scene.onUpdate(1.F);

    QCOMPARE(e.getComponent<engine::Transform>().position, glm::vec2(1.F, 2.F));
}

// ── onRender ──────────────────────────────────────────────────────────────────

void SceneTest::renderSystemIteratesSprites()
{
    // Verify that every entity with Transform+Sprite is visible to the render
    // view — no GL context needed; we just check via the registry directly.
    engine::Scene scene;

    auto a = scene.createEntity("a");
    a.addComponent<engine::Transform>();
    a.addComponent<engine::Sprite>();

    auto b = scene.createEntity("b");
    b.addComponent<engine::Transform>();
    b.addComponent<engine::Sprite>();

    // Entity without Sprite — should not appear in the render view.
    auto c = scene.createEntity("c");
    c.addComponent<engine::Transform>();

    int count = 0;
    auto view = scene.registry().view<engine::Transform, engine::Sprite>();
    for ([[maybe_unused]] auto e : view) {
        ++count;
    }
    QCOMPARE(count, 2);
}

QTEST_GUILESS_MAIN(SceneTest)
#include "scenetest.moc"
