add_executable(example_01_window)
target_sources(example_01_window PRIVATE main.cpp)
target_link_libraries(example_01_window PRIVATE engine Qt::Core)
