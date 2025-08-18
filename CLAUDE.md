# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Cook is a C++ application framework with graphics/rendering capabilities, featuring a modular architecture with CUDA support. The project uses Premake5 for build configuration and targets Windows x64 with Visual Studio 2019/2022.

## Build Commands

### Generate Visual Studio Solution
```bash
# Primary method (VS2022)
generateVS2022.bat

# For unit tests
cd unittest && generateVS2022.bat
```

### Build Project
```powershell
# Using Claude integration script (recommended - handles errors automatically)
powershell scripts/claude_build_command.ps1 -Action build

# Direct MSBuild
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" builds\VisualStudio2022\Apps.sln /p:Configuration=Debug /p:Platform=x64
```

### CUDA/PTX Embedding (if working with GPU code)
```bash
scripts/desktop_dog_embed_ptx_debug.bat    # Debug
scripts/desktop_dog_embed_ptx_release.bat  # Release
```

## High-Level Architecture

### Core Structure
- **appCore/**: Main application framework with Jahley engine
  - Precompiled header: `berserkpch.h`
  - Entry point and app configuration in `source/jahley/`
  - GUI components with OpenGL renderer

- **modules/**: Reusable components
  - `mace_core/`: Basic utilities and defaults
  - `oiio_core/`: OpenImageIO integration for image I/O
  - `sabi_core/`: Scene management, animation, model loading (GLTF, LWO3)
  - `wabi_core/`: Math utilities and spatial acceleration structures

- **framework/**: Core framework systems
  - `dog_core/`: Advanced GPU rendering with CUDA/OptiX integration
  - `properties_core/`: Property management system
  - `qms_core/`: QuickSilver Messenger Service for inter-component communication
  - `server_core/`: Socket server implementation

### Build System Architecture
- **Premake5** generates Visual Studio solutions
- **Configuration**: Debug/Release for x64
- **Output**: `builds/bin/` (binaries) and `builds/bin-int/` (intermediates)
- **Static linking** with runtime libraries
- **Precompiled headers** for faster compilation

### Key Design Patterns
- **Module separation**: Files in `excludeFromBuild/` folders are excluded from compilation
- **Resource management**: Resources stored per-app in `resources/[AppName]/`
- **Path handling**: Repository-relative paths using `REPOSITORY_NAME = "Cook"`
- **Message passing**: QMS (QuickSilver Messenger Service) for component communication

## Code Style

- **C++ Standard**: C++17 (VS2019) or C++20 (VS2022)
- **Naming**: PascalCase for classes, camelCase for functions/variables
- **Headers**: Use `#pragma once`
- **Paths**: Always use forward slashes, convert Windows paths as needed
- **Precompiled header**: Include `berserkpch.h` in implementation files

## Testing and Validation

### After Making Changes
1. Build the project: `powershell scripts/claude_build_command.ps1 -Action build`
2. Check for errors in `build_errors.txt` if build fails
3. Run unit tests if applicable: `builds/bin/Debug-windows-x86_64/HelloTest/HelloTest.exe`
4. Test sample apps: `builds/bin/Debug-windows-x86_64/HelloWorld/HelloWorld.exe`

### Common Build Issues
- Missing includes: Check precompiled header and include paths
- Undefined symbols: Verify library linking in premake5.lua files
- CUDA errors: Run PTX embedding scripts for GPU projects

## Important Notes

- The project extensively uses third-party libraries in `thirdparty/`
- CUDA/OptiX support requires PTX embedding via Python scripts
- Build errors are automatically formatted for Claude via PowerShell script
- Each module/framework component has its own excluded source folder pattern
- The messaging system (QMS) enables loose coupling between components

### Logging
- Use g3log for logging
- Use DBUG level for debug information (not DEBUG)
- Use

# C++ Development Guidelines

This file provides comprehensive guidance to Claude Code when working with C++ code in this repository.

## Core Development Philosophy

### KISS (Keep It Simple, Stupid)
Simplicity should be a key goal in design. Choose straightforward solutions over complex ones whenever possible. Simple solutions are easier to understand, maintain, and debug.

### YAGNI (You Aren't Gonna Need It)
Avoid building functionality on speculation. Implement features only when they are needed, not when you anticipate they might be useful in the future.

### Design Principles
- **Dependency Inversion**: High-level modules should not depend on low-level modules. Both should depend on abstractions (interfaces/abstract classes).
- **Open/Closed Principle**: Software entities should be open for extension but closed for modification.
- **Single Responsibility**: Each function, class, and translation unit should have one clear purpose.
- **Fail Fast**: Use assertions, exceptions, and early validation to catch errors immediately when issues occur.

## 🧱 Code Structure & Modularity

### File and Function Limits
- **Header files should be under 300 lines** and source files **under 500 lines of code**. If approaching these limits, refactor by splitting into multiple translation units.
- **Functions should be under 50 lines** with a single, clear responsibility.
- **Classes should be under 150 lines** and represent a single concept or entity.
- **Organize code into clearly separated header/source pairs**, grouped by feature or responsibility.
- **Line length should be max 100 characters** - configure clang-format or your formatter accordingly.
- **Use the project's build system** (CMake/Make/etc.) whenever compiling and testing, including for unit tests.

### C++ Specific Guidelines
- **Follow RAII principles** - manage resources through constructors/destructors
- **Prefer stack allocation** over dynamic allocation when possible
- **Use smart pointers** (`std::unique_ptr`, `std::shared_ptr`) for dynamic memory management
- **Include guards or `#pragma once`** in all header files
- **Forward declarations** in headers when possible to reduce compilation dependencies
- **Const-correctness** - mark methods and parameters const when they don't modify state