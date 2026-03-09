add_executable(breakout)
target_sources(breakout PRIVATE main.cpp breakoutlayer.cpp)
target_link_libraries(breakout PRIVATE engine Qt::Core)

qt_add_resources(breakout "breakout_assets"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        sprites/Background1.png
        sprites/Ball_small-blue.png
        sprites/Brick_unbreakable2.png
        sprites/Brick1_4.png
        sprites/Brick2_4.png
        sprites/Brick3_4.png
        sprites/Brick4_4.png
        sprites/Brick5_4.png
        sprites/Brick6_4.png
        sprites/Brick7_4.png
        sprites/Brick8_4.png
        sprites/Brick9_4.png
        sprites/Frame.png
        sprites/Player_flash.png
        sprites/Player.png
)
