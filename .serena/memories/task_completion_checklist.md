# Task Completion Checklist

When completing a coding task in the Cook project, ensure the following:

## Build Verification
1. **Regenerate Solution** (if project structure changed):
   ```bash
   generateVS2022.bat
   ```

2. **Build Debug Configuration**:
   ```powershell
   powershell scripts/claude_build_command.ps1 -Action build
   ```
   - Fix any compilation errors
   - Resolve any linker errors

3. **Build Release Configuration** (for production-ready code):
   ```bash
   MSBuild builds\VisualStudio2022\Apps.sln /p:Configuration=Release /p:Platform=x64
   ```

## Code Quality Checks
1. **Include Guards**: Ensure all headers use `#pragma once`
2. **Precompiled Headers**: Include `berserkpch.h` where appropriate
3. **Path Handling**: Use forward slashes, handle Windows paths correctly
4. **Memory Safety**: Check for potential memory leaks or access violations

## Testing
1. **Run Unit Tests** (if applicable):
   ```bash
   builds/bin/Debug-windows-x86_64/HelloTest/HelloTest.exe
   ```

2. **Run Sample Applications** to verify functionality:
   ```bash
   builds/bin/Debug-windows-x86_64/HelloWorld/HelloWorld.exe
   ```

## CUDA/OptiX Projects
If working with CUDA or OptiX code:
1. **Embed PTX files**:
   ```bash
   scripts/desktop_dog_embed_ptx_debug.bat  # for Debug
   scripts/desktop_dog_embed_ptx_release.bat  # for Release
   ```

## Documentation
1. Update relevant comments in code
2. Ensure function/class purposes are clear
3. Document any new build requirements or dependencies

## Pre-Commit
1. **Check Git Status**:
   ```bash
   git status
   ```
2. Review all changes for unintended modifications
3. Ensure no sensitive information in code
4. Verify no debug code or temporary files included

## Error Resolution
If build errors occur:
1. Check `build_errors.txt` for detailed error messages
2. Common issues to check:
   - Missing includes
   - Undefined symbols
   - Mismatched function signatures
   - Missing library links in premake configuration