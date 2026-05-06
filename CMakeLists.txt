# -------------------------------------------------------------------------------------------------
# PROJECT   : OpenView3D
# PURPOSE   : Modern C++ / OpenGL / Qt QML based 3D model viewer
# -------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(OpenView3D
    VERSION 0.1.0
    DESCRIPTION "A lightweight OpenGL-based 3D model viewer with Qt/QML interface"
    LANGUAGES CXX
)

# -------------------------------------------------------------------------------------------------
# C++ Standard
# -------------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# -------------------------------------------------------------------------------------------------
# Platform Settings
# -------------------------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION=1)
endif()

# -------------------------------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

qt_standard_project_setup(REQUIRES 6.5)

# Vendored dependencies under thirdparty/
add_subdirectory(thirdparty)

# -------------------------------------------------------------------------------------------------
# Source Files
# -------------------------------------------------------------------------------------------------
set(OPENVIEW3D_HEADERS
    "${CMAKE_SOURCE_DIR}/src/core/openGLWindow.h"
    "${CMAKE_SOURCE_DIR}/src/core/controls.h"
    "${CMAKE_SOURCE_DIR}/src/core/loader.h"
    "${CMAKE_SOURCE_DIR}/src/core/shader.h"
    "${CMAKE_SOURCE_DIR}/src/core/texture.h"
    "${CMAKE_SOURCE_DIR}/src/core/renderer.h"
    "${CMAKE_SOURCE_DIR}/src/ui/OpenGLViewport.h"
    "${CMAKE_SOURCE_DIR}/src/ui/AppController.h"

)

set(OPENVIEW3D_SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/openGLWindow.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/controls.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/loader.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/shader.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/texture.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/renderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/OpenGLViewport.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/AppController.cpp"
)

# -------------------------------------------------------------------------------------------------
# Executable Target
# -------------------------------------------------------------------------------------------------
qt_add_executable(OpenView3D
    ${OPENVIEW3D_HEADERS}
    ${OPENVIEW3D_SOURCES}
)

# -------------------------------------------------------------------------------------------------
# QML Module
# Required for:
#   engine.loadFromModule("OpenView3D", "Main");
# -------------------------------------------------------------------------------------------------
qt_add_qml_module(OpenView3D
    URI OpenView3D
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/icons/cube.svg
        qml/icons/texture.svg
        qml/icons/reset.svg
        qml/icons/zoom.svg
        qml/icons/rotate.svg
        qml/icons/cursor.svg
)


# -------------------------------------------------------------------------------------------------
# Generated Config Header
# -------------------------------------------------------------------------------------------------
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/config.h.in"
    "${CMAKE_BINARY_DIR}/config/config.h"
)

target_sources(OpenView3D PRIVATE
    "${CMAKE_BINARY_DIR}/config/config.h"
)

# -------------------------------------------------------------------------------------------------
# Include Directories
# -------------------------------------------------------------------------------------------------
target_include_directories(OpenView3D PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/thirdparty/glfw-3.3.6/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/glew-1.13.0/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/glm-0.9.7.1"
    "${CMAKE_SOURCE_DIR}/thirdparty/fast_obj"
    "${CMAKE_BINARY_DIR}/config"
)

if(APPLE)
    target_include_directories(OpenView3D PRIVATE
        "${OPENGL_INCLUDE_DIR}/OpenGL"
    )
else()
    target_include_directories(OpenView3D PRIVATE
        "${OPENGL_INCLUDE_DIR}/GL"
    )
endif()

# -------------------------------------------------------------------------------------------------
# Link Libraries
# -------------------------------------------------------------------------------------------------
target_link_libraries(OpenView3D PRIVATE
    OpenGL::GL
    glfw
    libglew_static

    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# -------------------------------------------------------------------------------------------------
# Compiler Warnings
# -------------------------------------------------------------------------------------------------
if(MSVC)
    target_compile_options(OpenView3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(OpenView3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -------------------------------------------------------------------------------------------------
# Windows Qt Deployment Helper
# -------------------------------------------------------------------------------------------------
if(WIN32)
    set_target_properties(OpenView3D PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()