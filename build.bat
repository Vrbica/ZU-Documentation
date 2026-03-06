@echo off
:: ============================================================
::  ZU-Documentation — one-click build script
::  Edit QT_DIR below to match your Qt installation path.
:: ============================================================

set QT_DIR=C:\Qt\6.7.0\msvc2019_64

:: Allow override from command line:  build.bat C:\Qt\6.8.0\msvc2022_64
if not "%1"=="" set QT_DIR=%1

set Qt6_DIR=%QT_DIR%\lib\cmake\Qt6
set PATH=%QT_DIR%\bin;%PATH%

echo.
echo ========================================
echo  Qt path : %QT_DIR%
echo ========================================

:: Configure
cmake -S . -B build\release ^
      -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DQt6_DIR="%Qt6_DIR%"
if errorlevel 1 ( echo [ERROR] CMake configure failed. & pause & exit /b 1 )

:: Build
cmake --build build\release --config Release
if errorlevel 1 ( echo [ERROR] Build failed. & pause & exit /b 1 )

echo.
echo ========================================
echo  Build succeeded!
echo  Executable: build\release\Release\ZU_Documentation.exe
echo ========================================
pause
