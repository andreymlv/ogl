find_package(Qt6 COMPONENTS Network REQUIRED)

add_executable(pong)
target_sources(pong PRIVATE main.cpp ponglayer.cpp)
target_link_libraries(pong PRIVATE
    engine
    Qt::Core
    Qt::Network
)

# miniaudio is header-only — include path comes from vcpkg toolchain automatically.
