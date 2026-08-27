@echo off
cd /d "%~dp0bin"

echo ========================================
echo   Creating GIF animation...
echo ========================================
echo.

:: Шаг 1: Анализируем кадры и генерируем идеальную цветовую палитру
ffmpeg -y -framerate 50 -i frames/frame_%%d.png -vf "palettegen" palette.png

:: Шаг 2: Сшиваем кадры в GIF, используя эту палитру (это уберет мыло и сохранит резкость пикселей)
ffmpeg -y -framerate 50 -i frames/frame_%%d.png -i palette.png -filter_complex "paletteuse" record.gif

:: Удаляем временный файл палитры
if exist palette.png del palette.png

:: Проверяем, создался ли итоговый GIF
if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo   The GIF has been successfully created: bin\record.gif
    echo ========================================
) else (
    echo.
    echo ========================================
    echo   Error while compiling GIF!
    echo ========================================
)

echo.
pause
