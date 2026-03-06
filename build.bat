@echo off
:: ============================================================
::  ZU-Documentation — one-click build script
::  Edit QT_DIR below if your Qt is installed elsewhere.
:: ============================================================

set QT_DIR=C:\Qt\6.10.2\msvc2022_64

:: Allow override from command line:  build.bat C:\Qt\6.8.0\msvc2022_64
if not "%1"=="" set QT_DIR=%1

set Qt6_DIR=%QT_DIR%\lib\cmake\Qt6
set PATH=%QT_DIR%\bin;%PATH%

:: ---- Locate cmake.exe ----
:: 1) Check if already on PATH
where cmake >nul 2>&1
if not errorlevel 1 goto :cmake_found

:: 2) Visual Studio 2022 bundled CMake (Community / Professional / Enterprise)
set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

:: 3) Qt Creator ships CMake at Qt\Tools\CMake_64\bin
set QT_CMAKE=C:\Qt\Tools\CMake_64\bin
if exist "%QT_CMAKE%\cmake.exe" ( set PATH=%QT_CMAKE%;%PATH% & goto :cmake_found )

echo [ERROR] cmake.exe not found.
echo.
echo   Options:
echo   a) Install CMake from https://cmake.org/download/ and tick "Add to PATH"
echo   b) Install "C++ CMake tools" workload in Visual Studio Installer
echo   c) Install Qt Creator (it includes CMake)
echo.
pause
exit /b 1

:cmake_found
echo.
echo ========================================
echo  Qt path  : %QT_DIR%
echo  CMake    : found
echo ========================================

:: Configure
cmake -S . -B build\release ^
      -G "Visual Studio 17 2022" -A x64 ^
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
