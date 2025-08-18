# Suggested Commands for Cook Project

## Build Commands

### Generate Visual Studio Solution
```bash
# For Visual Studio 2022 (recommended)
generateVS2022.bat

# From buildTools directory
buildTools/premake5.exe vs2022

# For unit tests
cd unittest
generateVS2022.bat
```

### Build Project
```powershell
# Using MSBuild directly (Visual Studio 2022)
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" builds\VisualStudio2022\Apps.sln /p:Configuration=Debug /p:Platform=x64

# Using Claude integration script (automatically handles errors)
powershell scripts/claude_build_command.ps1 -Action build

# Build loop mode (waits for fixes after errors)
powershell scripts/claude_build_command.ps1 -Action loop
```

### CUDA/PTX Embedding
```bash
# Debug build
scripts/desktop_dog_embed_ptx_debug.bat

# Release build
scripts/desktop_dog_embed_ptx_release.bat

# Manual PTX embedding
python scripts/embed_ptx.py
```

## Development Commands

### Running Applications
```bash
# Run HelloWorld application (after building)
builds/bin/Debug-windows-x86_64/HelloWorld/HelloWorld.exe

# Run unit tests
builds/bin/Debug-windows-x86_64/HelloTest/HelloTest.exe
```

### Clean Build
```bash
# Remove build artifacts
rmdir /s /q builds\bin
rmdir /s /q builds\bin-int

# Regenerate solution
generateVS2022.bat
```

## Git Commands
```bash
# Check status
git status

# Stage changes
git add .

# Commit changes
git commit -m "Your commit message"

# View recent commits
git log --oneline -10
```

## File System Navigation
```bash
# Common directories
cd appCore/source       # Core application source
cd appSandbox          # Sample applications
cd modules             # Reusable modules
cd framework           # Framework components
cd thirdparty         # External dependencies
cd unittest/tests     # Unit tests
```

## Debugging
```bash
# Check build errors
type build_errors.txt

# Find files
dir /s /b *.cpp | findstr "keyword"
dir /s /b *.h | findstr "keyword"
```