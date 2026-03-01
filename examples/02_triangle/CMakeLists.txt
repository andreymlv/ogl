add_executable(example_02_triangle)
target_sources(example_02_triangle PRIVATE main.cpp)
target_link_libraries(example_02_triangle PRIVATE engine Qt::Core)
