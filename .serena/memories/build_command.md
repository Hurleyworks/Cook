# Build Command for Cook Project

When building the Cook project in Claude Code on Windows/WSL, use:

```bash
cmd.exe /c b.bat
```

This runs the Windows batch file that properly invokes MSBuild. Do NOT use:
- `cmd /c b.bat` (missing .exe)
- `./b.bat` (tries to run as bash script)
- `powershell scripts/claude_build_command.ps1` (different command)

The b.bat file handles error formatting and creates build_errors.txt if the build fails.