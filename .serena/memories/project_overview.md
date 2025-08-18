# Cook Project Overview

## Purpose
Cook is a C++ application framework with a focus on graphics/rendering, featuring:
- Core application framework (AppCore)
- Module-based architecture with various cores (mace, oiio, sabi, wabi)
- Framework components for rendering, properties, messaging, and server functionality
- Extensive third-party library integration
- CUDA support for GPU computation
- Unit testing framework

## Tech Stack
- **Language**: C++ (C++17/C++20 depending on VS version)
- **Build System**: Premake5 with custom CUDA support
- **Target Platform**: Windows x64 (Visual Studio 2019/2022)
- **Graphics**: OpenGL, GLFW, NanoGUI, CUDA/OptiX
- **Architecture**: AVX2 vectorization
- **Key Libraries**:
  - Graphics: GLFW, NanoGUI, OpenGL, OptiX
  - Logging: g3log
  - Testing: doctest, benchmark
  - Utilities: cpptrace, libassert, fastgltf, Eigen
  - Image I/O: OpenImageIO (OIIO)

## Repository Structure
- `appCore/`: Core application framework with Jahley engine
- `appSandbox/`: Sample applications (e.g., HelloWorld)
- `buildTools/`: Premake configuration and build utilities
- `framework/`: Core framework components
  - `bare_bones_render_core/`: Basic rendering core
  - `dog_core/`: Advanced rendering with CUDA/OptiX
  - `properties_core/`: Properties management
  - `qms_core/`: QuickSilver Messenger Service (messaging system)
  - `server_core/`: Socket server implementation
- `modules/`: Reusable modules
  - `mace_core/`: Basic utilities and defaults
  - `oiio_core/`: OpenImageIO integration
  - `sabi_core/`: Scene, animation, and I/O (supports GLTF, LWO3)
  - `wabi_core/`: Math and geometry acceleration structures
- `resources/`: Application resources
- `scripts/`: Build and utility scripts
- `thirdparty/`: External dependencies
- `unittest/`: Unit test projects

## Build Configuration
- Uses precompiled headers (berserkpch.h)
- Multi-processor compilation
- Static runtime linking
- Separate Debug/Release configurations
- Output directories: `builds/bin/` and `builds/bin-int/`