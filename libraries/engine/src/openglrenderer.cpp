#include "openglrenderer.h"

#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <QLoggingCategory>

#include "engine_logging.h"

namespace engine
{

class OpenGLRendererPrivate
{
public:
    bool m_valid = false;
};

OpenGLRenderer::OpenGLRenderer()
    : d(std::make_unique<OpenGLRendererPrivate>())
{
}

OpenGLRenderer::~OpenGLRenderer() = default;

bool OpenGLRenderer::init(Window &window)
{
    Q_UNUSED(window)

    // The OpenGL function pointer loader belongs here — the renderer owns the
    // graphics API. GlfwWindow only creates the OS surface and makes the
    // context current; it knows nothing about which API fills it.
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        qCCritical(Engine) << "Failed to initialize GLAD";
        return false;
    }

    d->m_valid = true;
    return true;
}

bool OpenGLRenderer::isValid() const
{
    return d->m_valid;
}

void OpenGLRenderer::beginFrame()
{
    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::endFrame()
{
    // Buffer swap is the window's responsibility — it owns the surface.
}

void OpenGLRenderer::onResize(int width, int height)
{
    glViewport(0, 0, width, height);
}

} // namespace engine
