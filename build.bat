@echo off
cd /d "%~dp0"
if not exist bin mkdir bin
if not exist libs mkdir libs
echo Compiling...
echo.

g++ -std=c++20 -O2 -fopenmp ^
src\universe.cpp ^
src\particle.cpp ^
src\config_loader.cpp ^
src\analyzer.cpp ^
src\galaxy_saver.cpp ^
src\spatial_grid.cpp ^
src\physics.cpp ^
src\physics_laws.cpp ^
src\main.cpp ^
src\nbody_solver.cpp ^
src\barnes_hut.cpp ^
src\opencl_manager.cpp ^
src\vec3.cpp ^
src\visualizer.cpp ^
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