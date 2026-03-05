cmake_minimum_required(VERSION 4.2.3)

if (CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
    set(CMAKE_PROJECT_ogl_INCLUDE_BEFORE "meta/vcpkg/generate_vcpkg_toolchain_variables.cmake")
endif()

project(ogl
  VERSION 0.0.1
  LANGUAGES C CXX
)

find_package(ECM 6.23 REQUIRED NO_MODULE)
list(PREPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)

find_package(Qt6 COMPONENTS Core Test Network)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(ECMEnableSanitizers)
include(ECMSetupVersion)
include(ECMGenerateExportHeader)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(ECMCoverageOption)

if(BUILD_COVERAGE)
    find_program(GCOVR gcovr)
    if(GCOVR)
        set(COVERAGE_HTML_DIR "${CMAKE_BINARY_DIR}/coverage-report")
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_HTML_DIR}"
            COMMAND ${GCOVR}
                --root "${CMAKE_SOURCE_DIR}"
                --object-directory "${CMAKE_BINARY_DIR}"
                --gcov-executable "llvm-cov gcov"
                --exclude ".*autotests.*"
                --exclude ".*vcpkg.*"
                --html-details "${COVERAGE_HTML_DIR}/index.html"
                --print-summary
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Generating coverage report -> ${COVERAGE_HTML_DIR}/index.html  (run ctest first)"
            VERBATIM
        )
    else()
        message(STATUS "gcovr not found — 'coverage' target unavailable; install gcovr to generate HTML reports")
    endif()
endif()

add_subdirectory(libraries)
add_subdirectory(games)
add_subdirectory(examples)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES
    libraries/*.cpp libraries/*.h
    games/*.cpp games/*.h
    examples/*.cpp examples/*.h
)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
