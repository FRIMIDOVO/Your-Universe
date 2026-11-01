@echo off
cd /d "%~dp0"

echo ========================================
echo   Building EXE with PyInstaller...
echo ========================================

pyinstaller --noconfirm --clean --windowed --onedir ^
    --contents-directory "." ^
    --add-data "bin;bin" ^
    --add-data "gui;gui" ^
    --name "Universe" ^
    main.py

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo   PyInstaller failed!
    echo ========================================
    pause
    exit /b
)

echo.
echo ========================================
echo   Build successful!
echo ========================================
pause