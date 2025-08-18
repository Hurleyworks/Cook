@echo off
echo Running new embed_ptx.py...
"C:\Users\Steve\AppData\Local\Programs\Python\Python311\python.exe" "E:\1Cook\Cook\scripts\embed_ptx.py" ^
"E:\1Cook\Cook\resources\RenderDog\ptx" "E:\1Cook\Cook\framework\dog_core\generated\embedded_ptx.h" "Debug"
echo Exit code: %ERRORLEVEL%
echo Checking if script completed...
if exist "E:\1Cook\Cook\framework\dog_core\generated\embedded_ptx.h" (
  echo SUCCESS: Output file was created
) else (
  echo ERROR: Output file was not created
)
pause


