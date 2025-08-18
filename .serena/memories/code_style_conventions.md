# Code Style and Conventions

## C++ Standards
- **Visual Studio 2019**: C++17
- **Visual Studio 2022**: C++20

## Naming Conventions
- **Classes/Structs**: PascalCase (e.g., `AppCore`, `WorldItem`)
- **Functions**: camelCase (e.g., `getExecutablePath`, `getResourcePath`)
- **Member Variables**: camelCase, often with descriptive names
- **Constants**: UPPER_SNAKE_CASE (e.g., `REPOSITORY_NAME`)
- **Namespaces**: lowercase
- **File Names**: 
  - Headers: PascalCase.h (e.g., `App.h`, `OpenglRenderer.h`)
  - Source: PascalCase.cpp
  - Precompiled headers: lowercase (e.g., `berserkpch.h`)

## Code Organization
- **Precompiled Headers**: Use `berserkpch.h` for common includes
- **Header Guards**: `#pragma once` preferred
- **Include Order**: 
  1. Precompiled header
  2. Local headers
  3. Module headers
  4. Third-party headers
  5. System headers

## Directory Structure
- Source files in `source/` subdirectories
- Headers alongside implementation files
- Use `excludeFromBuild/` for files to exclude from compilation

## Build Configuration
- **Defines**: 
  - Debug: `DEBUG`, `USE_DEBUG_EXCEPTIONS`
  - Release: `NDEBUG`
  - Windows: `WIN32`, `_WINDOWS`, `NOMINMAX`
  - Various library-specific defines

## Platform-Specific Code
- Windows-specific code uses `#ifdef _WIN32`
- Use forward slashes for paths, convert backslashes when needed
- Character set: MBCS (Multi-Byte Character Set)

## Memory Management
- Static runtime linking (`staticruntime "On"`)
- Aligned storage enabled (`_ENABLE_EXTENDED_ALIGNED_STORAGE`)

## Error Handling
- Use assertions with libassert (`LIBASSERT_LOWERCASE`)
- Debug exceptions enabled in Debug builds
- g3log for logging with dynamic logging enabled

## Warnings
- Disabled warnings: 5030, 4244, 4267, 4667, 4018, 4101, 4305, 4316, 4146, 4996
- Linker ignore: 4099 (missing PDB files)

## Build Options
- Multi-processor compilation (`/MP`)
- Large object files (`/bigobj`)
- Increased precompiled header memory (`/Zm250`)
- C++ standard compliance (`/Zc:__cplusplus`)