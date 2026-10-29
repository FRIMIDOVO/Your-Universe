@echo off
cd /d "%~dp0"
if not exist bin mkdir bin
if not exist libs mkdir libs
echo Compiling...
echo.

g++ -std=c++20 -O2 -fopenmp ^
scr\particle.cpp ^
scr\config_loader.cpp ^
scr\galaxy_saver.cpp ^
scr\spatial_grid.cpp ^
scr\physics.cpp ^
scr\physics_laws.cpp ^
scr\main.cpp ^
scr\nbody_solver.cpp ^
scr\barnes_hut.cpp ^
scr\opencl_manager.cpp ^
scr\vec3.cpp ^
scr\visualizer.cpp ^
-Iinclude ^
-Iinclude/Raylib ^
-Llibs ^
-lOpenCL ^
-lraylibdll ^
-lgdi32 ^
-lwinmm ^
-lopengl32 ^
-o bin\universe.exe

if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo   Build successful!
    echo ========================================
    echo.
) else (
    echo.
    echo ========================================
    echo   Build failed!
    echo ========================================
)
pause