# Cook

A modular C++ application framework with advanced graphics and rendering capabilities, featuring CUDA/OptiX support and a comprehensive module system.

## Features

- **Modular Architecture**: Organized into reusable modules and framework components
- **GPU Computing**: CUDA and OptiX integration for high-performance rendering
- **Graphics Pipeline**: OpenGL-based rendering with GLFW and NanoGUI
- **Scene Management**: Complete scene graph with animation and model loading support (GLTF, LWO3)
- **Messaging System**: QuickSilver Messenger Service (QMS) for component communication
- **Image Processing**: OpenImageIO integration for comprehensive image format support
- **Math & Geometry**: Extensive math utilities and spatial acceleration structures
- **Cross-Platform Build**: Premake5-based build system with Visual Studio support

## Prerequisites

- Windows 10/11 (x64)
- Visual Studio 2019 or 2022
- CUDA Toolkit (optional, for GPU features)
- Python 3.x (for PTX embedding scripts)

## Quick Start

### 1. Generate Visual Studio Solution

```bash
# Generate VS2022 solution
generateVS2022.bat
```

### 2. Build the Project

```powershell
# Using the Claude integration script (recommended)
powershell scripts/claude_build_command.ps1 -Action build

# Or using MSBuild directly
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" builds\VisualStudio2022\Apps.sln /p:Configuration=Debug /p:Platform=x64
```

### 3. Run Sample Application

```bash
builds/bin/Debug-windows-x86_64/HelloWorld/HelloWorld.exe
```

## Project Structure

```
Cook/
├── appCore/              # Core application framework
│   └── source/
│       ├── jahley/       # Main engine components
│       └── berserkpch.h  # Precompiled header
├── appSandbox/           # Sample applications
│   └── HelloWorld/       # Basic example app
├── buildTools/           # Build configuration and utilities
├── framework/            # Core framework components
│   ├── dog_core/         # GPU rendering (CUDA/OptiX)
│   ├── properties_core/  # Property management
│   ├── qms_core/         # Messaging system
│   └── server_core/      # Network server
├── modules/              # Reusable modules
│   ├── mace_core/        # Basic utilities
│   ├── oiio_core/        # Image I/O (OpenImageIO)
│   ├── sabi_core/        # Scene and animation
│   └── wabi_core/        # Math and geometry
├── resources/            # Application resources
├── scripts/              # Build and utility scripts
├── thirdparty/           # External dependencies
└── unittest/             # Unit test framework
```

## Modules

### Core Modules

- **mace_core**: Foundation utilities, input handling, and default configurations
- **oiio_core**: OpenImageIO wrapper for comprehensive image format support
- **sabi_core**: Scene management, animation, camera systems, and 3D model loading
- **wabi_core**: Mathematical operations, geometric algorithms, and acceleration structures

### Framework Components

- **dog_core**: Advanced GPU-accelerated rendering with CUDA and OptiX
- **properties_core**: Flexible property system for configuration management
- **qms_core**: QuickSilver Messenger Service for event-driven architecture
- **server_core**: Socket-based server implementation for network communication
- **bare_bones_render_core**: Simplified rendering pipeline for basic graphics

## Building with CUDA/OptiX

For GPU-accelerated features:

```bash
# Embed PTX for Debug build
scripts/desktop_dog_embed_ptx_debug.bat

# Embed PTX for Release build
scripts/desktop_dog_embed_ptx_release.bat
```

## Unit Testing

```bash
# Generate test solution
cd unittest
generateVS2022.bat

# Build and run tests
builds/bin/Debug-windows-x86_64/HelloTest/HelloTest.exe
```

## Development

### Code Style

- **C++ Standard**: C++17 (VS2019) / C++20 (VS2022)
- **Naming**: PascalCase for types, camelCase for functions/variables
- **Headers**: Use `#pragma once` for include guards

### Creating a New Application

1. Create a new folder in `appSandbox/`
2. Add a `premake5.lua` configuration file
3. Include the project in the main `premake5.lua`
4. Regenerate the solution with `generateVS2022.bat`

### Adding a Module

1. Create a module folder in `modules/` or `framework/`
2. Place implementation in `excludeFromBuild/` subdirectory
3. Add module headers and implementation files
4. Update include paths in dependent projects

## Dependencies

Major third-party libraries included:

- **Graphics**: GLFW, NanoGUI, OpenGL
- **GPU**: CUDA, OptiX
- **Image I/O**: OpenImageIO, STB Image
- **Math**: Eigen, GLM
- **Utilities**: g3log (logging), cpptrace (stack traces), doctest (testing)
- **Serialization**: Cereal, RapidJSON
- **Networking**: Custom socket implementation

## Troubleshooting

### Build Errors

1. Check `build_errors.txt` for detailed error messages
2. Ensure all prerequisites are installed
3. Verify Visual Studio version matches the generated solution

### Common Issues

- **Missing PTX files**: Run the appropriate PTX embedding script
- **Linker errors**: Check library paths in `premake5.lua`
- **Include errors**: Verify precompiled header is included

## License

[License information to be added]

## Contributing

[Contributing guidelines to be added]

## Support

For issues and questions, please check the `scripts/claude_build_command.ps1` script which provides automated error handling and integration with Claude AI for debugging assistance.