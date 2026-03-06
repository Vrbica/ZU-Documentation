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
where cmake >nul 2>&1
if not errorlevel 1 goto :cmake_found

set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

set VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
if exist "%VS_CMAKE%\cmake.exe" ( set PATH=%VS_CMAKE%;%PATH% & goto :cmake_found )

set QT_CMAKE=C:\Qt\Tools\CMake_64\bin
if exist "%QT_CMAKE%\cmake.exe" ( set PATH=%QT_CMAKE%;%PATH% & goto :cmake_found )

echo [ERROR] cmake.exe not found.
pause & exit /b 1

:cmake_found
echo.
echo ========================================
echo  Qt path  : %QT_DIR%
echo  CMake    : %QT_DIR%
cmake --version
echo ========================================

:: ---- MSVC environment (needed for cl.exe) ----
set VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VS_INSTALL=%%i
)
if defined VS_INSTALL (
    echo  VS install: %VS_INSTALL%
    call "%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
) else (
    echo [WARN] Could not find VS install via vswhere — cl.exe may be missing
)
echo.

:: ---- Configure ----
echo [1/2] Configuring...
cmake -S . -B build\release ^
      -G "Visual Studio 17 2022" -A x64 ^
      -DQt6_DIR="%Qt6_DIR%" ^
      2>&1
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configure failed — see output above.
    pause & exit /b 1
)

:: ---- Build ----
echo.
echo [2/2] Building...
cmake --build build\release --config Release 2>&1
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed — see output above.
    pause & exit /b 1
)

echo.
echo ========================================
echo  Build succeeded!
echo  Executable: build\release\Release\ZU_Documentation.exe
echo ========================================
pause
