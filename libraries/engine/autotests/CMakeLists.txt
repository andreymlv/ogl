ecm_add_test(applicationtest.cpp
    TEST_NAME applicationtest
    LINK_LIBRARIES
        engine
        Qt::Test
)

ecm_add_test(shadertest.cpp
    TEST_NAME shadertest
    LINK_LIBRARIES
        engine
        Qt::Test
        glfw
        glad::glad
)

# GLFW on Wayland initialises D-Bus/XKB without a teardown path, which
# LeakSanitizer flags as a leak.  The allocations are in system libraries
# we do not control, so suppress LSan for this test only.
set_tests_properties(shadertest PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=detect_leaks=0"
)

ecm_add_test(renderer2dtest.cpp
    TEST_NAME renderer2dtest
    LINK_LIBRARIES
        engine
        Qt::Test
        glfw
        glad::glad
)

set_tests_properties(renderer2dtest PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=detect_leaks=0"
)

ecm_add_test(texturetest.cpp
    TEST_NAME texturetest
    LINK_LIBRARIES
        engine
        Qt::Test
        glfw
        glad::glad
)

set_tests_properties(texturetest PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=detect_leaks=0"
)

ecm_add_test(audiotest.cpp
    TEST_NAME audiotest
    LINK_LIBRARIES
        engine
        Qt::Test
)

set_tests_properties(audiotest PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=detect_leaks=0"
)

ecm_add_test(scenetest.cpp
    TEST_NAME scenetest
    LINK_LIBRARIES
        engine
        Qt::Test
)

ecm_add_test(camera2dtest.cpp
    TEST_NAME camera2dtest
    LINK_LIBRARIES
        engine
        Qt::Test
)

ecm_add_test(layertest.cpp
    TEST_NAME layertest
    LINK_LIBRARIES
        engine
        Qt::Test
)
