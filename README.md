# OpenView3D
OpenView3D

<img width="1278" height="786" alt="Open3dView_06-05-2026" src="https://github.com/user-attachments/assets/1bcd824b-6aa4-480c-86e2-9696c9590408" />

# OpenView3D

Modern Qt/QML + OpenGL based lightweight 3D model viewer.

OpenView3D is a desktop application built with:

* Qt 6
* QML
* OpenGL 3.3 Core
* C++
* GLM

The application provides a modern CAD-style interface for viewing and interacting with OBJ models in real time.

---

# Features

## Current Features

* OBJ model loading
* Real-time OpenGL rendering
* Modern QML UI
* Zoom controls
* X/Y rotation controls
* Camera reset
* Model path inspector
* UV buffer support
* Texture loading pipeline
* DDS/BMP texture infrastructure
* Responsive CAD-style viewport
* Custom sliders and tool panels
* SVG icon-based UI

---

# Screenshots

Current UI includes:

* Left tool sidebar
* Scene controls panel
* CAD-style viewport
* Interactive camera controls

---

# Technologies

## Frontend/UI

* Qt Quick / QML
* Qt Quick Controls Basic
* SVG assets
* Custom reusable QML components

## Rendering

* OpenGL 3.3 Core Profile
* QOpenGLShaderProgram
* QQuickFramebufferObject
* GLSL shaders

## Math / Utilities

* GLM
* Custom OBJ loader
* Custom texture loader

---

# Project Structure

```text
OpenView3D/
│
├── resources/
│   ├── models/
│   ├── shaders/
│   └── icons/
│
├── src/
│   ├── core/
│   │   ├── loader.cpp
│   │   ├── texture.cpp
│   │   ├── Renderer.cpp
│   │   └── shader.cpp
│   │
│   ├── ui/
│   │   ├── OpenGLViewport.cpp
│   │   ├── OpenGLViewport.h
│   │   ├── AppController.cpp
│   │   └── AppController.h
│   │
│   └── main.cpp
│
└── qml/
    ├── Main.qml
    └── icons/
```

---

# Current Rendering Pipeline

```text
OBJ Loader
    ↓
Vertex/UV Buffers
    ↓
OpenGL VAO/VBO
    ↓
GLSL Shader Pipeline
    ↓
QQuickFramebufferObject
    ↓
Qt Quick Scene
```

---

# Implemented Controls

| Control      | Description                |
| ------------ | -------------------------- |
| Zoom         | Camera distance            |
| Rotation X   | Vertical rotation          |
| Rotation Y   | Horizontal rotation        |
| Reset Camera | Restores default transform |

---

# Texture System Status

## Working

* Texture path propagation
* UV loading
* UV buffer upload
* Shader UV pipeline
* BMP texture support pipeline

## In Progress

* DDS texture stability
* Advanced material rendering
* Normal mapping
* Lighting system

---

# UI Design

The interface is inspired by modern CAD and DCC applications:

* Blender
* Plasticity
* Fusion 360
* Minimal industrial design systems

Color palette uses soft neutral tones and clean viewport contrast.

---

# Future Plans

## Rendering

* Full texture rendering 
* Lighting
* PBR materials
* Grid rendering
* Wireframe mode
* Skybox

## Interaction

* Mouse orbit camera
* Pan controls
* Scroll zoom
* Object selection

## File Support

* FBX
* GLTF
* STL

## UI

* Dockable panels
* Scene hierarchy
* Material inspector
* Asset browser

---

# Build Requirements

* Qt 6.11+
* MinGW 64-bit
* OpenGL 3.3+
* CMake

---

# Build

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

---

# Run

```bash
OpenView3D.exe
```

---

# Current Development Status

OpenView3D is currently in active development.

Core rendering and viewport systems are operational, while texture rendering and advanced shading systems are being integrated incrementally.

---

# License

______________

---

# Author

Busra / OpenView3D Project

