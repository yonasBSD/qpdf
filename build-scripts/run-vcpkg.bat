@echo on
@rem Usage: run-vcpkg.bat {x64-windows-static|x64-mingw-static|x86-windows-static|x86-mingw-static}
setlocal ENABLEDELAYEDEXPANSION
if "%1" == "x64-windows-static" (
   call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) else if "%1" == "x86-windows-static" (
   call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
)
set MSYS2_PATH_TYPE=inherit
C:\msys64\usr\bin\env.exe MSYSTEM=MINGW64 /bin/bash -l %CD%/build-scripts/run-vcpkg %1
