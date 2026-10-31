@echo off
cd /d "%~dp0"

echo ========================================
echo   Configuring CMake...
echo ========================================
:: Создаем папку build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo   CMake configuration failed!
    echo ========================================
    pause
    exit /b
)

echo.
echo ========================================
echo   Building...
echo ========================================
:: Собираем проект
cmake --build build --config Release

if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo   Build successful!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo   Build failed!
    echo ========================================
)
pause