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

include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(ECMEnableSanitizers)
include(ECMSetupVersion)
include(ECMGenerateExportHeader)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)

add_subdirectory(libraries)
add_subdirectory(games)
add_subdirectory(examples)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
