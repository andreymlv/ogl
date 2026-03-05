add_executable(flappybird)
target_sources(flappybird PRIVATE main.cpp flappylayer.cpp)
target_link_libraries(flappybird PRIVATE engine Qt::Core)

qt_add_resources(flappybird "flappybird_assets"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        sprites/background-day.png
        sprites/base.png
        sprites/pipe-green.png
        sprites/yellowbird-downflap.png
        sprites/yellowbird-midflap.png
        sprites/yellowbird-upflap.png
        sprites/message.png
        sprites/gameover.png
        sprites/0.png
        sprites/1.png
        sprites/2.png
        sprites/3.png
        sprites/4.png
        sprites/5.png
        sprites/6.png
        sprites/7.png
        sprites/8.png
        sprites/9.png
        audio/wing.wav
        audio/point.wav
        audio/hit.wav
        audio/die.wav
        audio/swoosh.wav
)
