add_library(engine)

target_sources(engine PRIVATE
    application.cpp
    glfwwindow.cpp
    openglrenderer.cpp
    renderer2d.cpp
    shader.cpp
)

ecm_generate_export_header(engine
    VERSION ${PROJECT_VERSION}
)

ecm_qt_declare_logging_category(engine
    HEADER engine_logging.h
    CATEGORY_NAME engine
    IDENTIFIER Engine
    EXPORT OGL
    DESCRIPTION "OGL Engine"
)

target_link_libraries(engine
    PUBLIC
        glfw
        glad::glad
        glm::glm
        Qt::Core
)

