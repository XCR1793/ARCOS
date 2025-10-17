@echo off
REM ARCOS Library Setup Script for PlatformIO (Windows)
REM This script helps set up the ARCOS library for PlatformIO projects

echo ARCOS Library PlatformIO Setup
echo ==============================

REM Check if we're in a PlatformIO project
if not exist "platformio.ini" (
    echo Error: No platformio.ini found. Please run this script in your PlatformIO project root.
    pause
    exit /b 1
)

REM Create lib directory if it doesn't exist
if not exist "lib" mkdir lib

REM Check if ARCOS is already cloned
if exist "lib\ARCOS" (
    echo ARCOS already exists in lib\. Updating...
    cd lib\ARCOS
    git pull
    cd ..\..
) else (
    echo Cloning ARCOS repository...
    cd lib
    git clone https://github.com/XCR1793/ARCOS.git
    cd ..
)

REM Check if library is properly structured
if exist "lib\ARCOS\ARCOS_library" (
    echo Repository structure detected. Setting up library...
    
    REM Option 1: Move library to proper location
    if not exist "lib\ARCOS_Library" (
        echo Creating properly structured library...
        xcopy "lib\ARCOS\ARCOS_library" "lib\ARCOS_Library\" /E /I /H /Y
        echo ✓ ARCOS library set up in lib\ARCOS_Library\
        
        echo.
        echo Setup complete! You can now use:
        echo #include ^<arcos.hpp^>
        echo.
        echo Or modular includes like:
        echo #include ^<arcos_core.hpp^>
        echo #include ^<arcos_algorithms.hpp^>
        echo #include ^<arcos_drivers.hpp^>
    ) else (
        echo ✓ ARCOS library already properly set up.
    )
    
) else (
    echo ✓ Library is already at repository root.
)

echo.
echo Alternative: Add this to your platformio.ini build_flags:
echo -I.pio/libdeps/$PIOENV/ARCOS/ARCOS_library/include
echo.
echo For Git dependency method, use:
echo lib_deps = https://github.com/XCR1793/ARCOS.git#main

pause