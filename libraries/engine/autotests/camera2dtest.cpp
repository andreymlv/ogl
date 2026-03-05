#include <QTest>

#include "camera2d.h"

#include <glm/gtc/matrix_transform.hpp>

// ── helpers ───────────────────────────────────────────────────────────────────

static bool mat4Near(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-5F)
{
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (std::abs(a[col][row] - b[col][row]) > eps) {
                return false;
            }
        }
    }
    return true;
}

// ── Test class ────────────────────────────────────────────────────────────────

class Camera2DTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Default state
    void defaultPositionIsZero();
    void defaultRotationIsZero();
    void defaultZoomIsOne();

    // Setters / getters
    void setPositionRoundTrips();
    void setRotationRoundTrips();
    void setZoomRoundTrips();

    // View-projection matrix correctness
    void identityProjectionAtOrigin();
    void translateMovesViewProjection();
    void zoomScalesViewProjection();
    void rotationRotatesViewProjection();
};

// ── default state ─────────────────────────────────────────────────────────────

void Camera2DTest::defaultPositionIsZero()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    QCOMPARE(cam.position(), glm::vec2(0.F, 0.F));
}

void Camera2DTest::defaultRotationIsZero()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    QCOMPARE(cam.rotation(), 0.F);
}

void Camera2DTest::defaultZoomIsOne()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    QCOMPARE(cam.zoom(), 1.F);
}

// ── setters ───────────────────────────────────────────────────────────────────

void Camera2DTest::setPositionRoundTrips()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setPosition({3.F, -7.F});
    QCOMPARE(cam.position(), glm::vec2(3.F, -7.F));
}

void Camera2DTest::setRotationRoundTrips()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setRotation(45.F);
    QCOMPARE(cam.rotation(), 45.F);
}

void Camera2DTest::setZoomRoundTrips()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setZoom(2.F);
    QCOMPARE(cam.zoom(), 2.F);
}

// ── view-projection matrix ────────────────────────────────────────────────────

void Camera2DTest::identityProjectionAtOrigin()
{
    // Camera at origin, no rotation, zoom=1.
    // VP = ortho(left,right,bottom,top) * identity_view
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    const glm::mat4 expected = glm::ortho(-1.F, 1.F, -1.F, 1.F);
    QVERIFY(mat4Near(cam.viewProjectionMatrix(), expected));
}

void Camera2DTest::translateMovesViewProjection()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setPosition({0.5F, 0.25F});

    // View matrix = translate by -position (camera moves world in opposite dir)
    const glm::mat4 proj = glm::ortho(-1.F, 1.F, -1.F, 1.F);
    const glm::mat4 view = glm::translate(glm::mat4(1.F), {-0.5F, -0.25F, 0.F});
    const glm::mat4 expected = proj * view;

    QVERIFY(mat4Near(cam.viewProjectionMatrix(), expected));
}

void Camera2DTest::zoomScalesViewProjection()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setZoom(2.F);

    const glm::mat4 proj = glm::ortho(-1.F, 1.F, -1.F, 1.F);
    const glm::mat4 view = glm::scale(glm::mat4(1.F), {2.F, 2.F, 1.F});
    const glm::mat4 expected = proj * view;

    QVERIFY(mat4Near(cam.viewProjectionMatrix(), expected));
}

void Camera2DTest::rotationRotatesViewProjection()
{
    engine::Camera2D cam(-1.F, 1.F, -1.F, 1.F);
    cam.setRotation(90.F);

    const glm::mat4 proj = glm::ortho(-1.F, 1.F, -1.F, 1.F);
    const glm::mat4 view = glm::rotate(glm::mat4(1.F), glm::radians(90.F), {0.F, 0.F, 1.F});
    const glm::mat4 expected = proj * view;

    QVERIFY(mat4Near(cam.viewProjectionMatrix(), expected));
}

QTEST_GUILESS_MAIN(Camera2DTest)
#include "camera2dtest.moc"
