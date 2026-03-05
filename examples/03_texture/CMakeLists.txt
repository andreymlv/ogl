add_executable(example_03_texture)
target_sources(example_03_texture PRIVATE main.cpp)
target_link_libraries(example_03_texture PRIVATE engine Qt::Core)

qt_add_resources(example_03_texture "texture_assets"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/libraries/engine/autotests"
    FILES
        ${CMAKE_SOURCE_DIR}/libraries/engine/autotests/container.jpg
        ${CMAKE_SOURCE_DIR}/libraries/engine/autotests/awesomeface.png
)
