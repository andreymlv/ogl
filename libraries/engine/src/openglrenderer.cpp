#include "openglrenderer.h"

#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <QLoggingCategory>

#include "engine_logging.h"

namespace engine
{

class OpenGLRendererPrivate
{
public:
    bool m_valid = false;
    bool m_imguiInitialized = false;
};

OpenGLRenderer::OpenGLRenderer()
    : d(std::make_unique<OpenGLRendererPrivate>())
{
}

OpenGLRenderer::~OpenGLRenderer()
{
    if (d->m_imguiInitialized) {
        qCDebug(Engine) << "Shutting down ImGui";
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

bool OpenGLRenderer::init(Window &window)
{
    // The OpenGL function pointer loader belongs here — the renderer owns the
    // graphics API. GlfwWindow only creates the OS surface and makes the
    // context current; it knows nothing about which API fills it.
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        qCCritical(Engine) << "Failed to initialize GLAD";
        return false;
    }

    d->m_valid = true;
    qCDebug(Engine) << "OpenGL initialized via GLAD — vendor:" << reinterpret_cast<const char *>(glGetString(GL_VENDOR))
                    << "renderer:" << reinterpret_cast<const char *>(glGetString(GL_RENDERER))
                    << "version:" << reinterpret_cast<const char *>(glGetString(GL_VERSION));

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto *glfwWindow = static_cast<GLFWwindow *>(window.nativeHandle());
    // Install callbacks = false: we drive GLFW polling ourselves, ImGui should
    // not install its own callbacks that could conflict with our event handling.
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    d->m_imguiInitialized = true;
    qCDebug(Engine) << "Dear ImGui initialized";

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

void OpenGLRenderer::beginImGui()
{
    if (!d->m_imguiInitialized) {
        return;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void OpenGLRenderer::endImGui()
{
    if (!d->m_imguiInitialized) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenGLRenderer::onResize(int width, int height)
{
    qCDebug(Engine) << "Viewport resized to" << width << "x" << height;
    glViewport(0, 0, width, height);
}

} // namespace engine
