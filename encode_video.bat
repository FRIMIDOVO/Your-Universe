@echo off
cd /d "%~dp0bin"

echo ========================================
echo   Creating video...
echo ========================================
echo.

:: Флаг -loglevel warning уберет весь мусор из консоли, оставив только важные ошибки
ffmpeg -y -framerate 60 -i frames/frame_%%d.png -c:v h264_nvenc -pix_fmt yuv420p -profile:v high -rc constqp -qp 1 -spatial-aq 0 -temporal-aq 0 -preset slow record.mp4

:: Явно проверяем, создался ли файл record.mp4
if %errorlevel% == 0 (
    echo.
    echo ========================================
    echo   The video has been successfully created: bin\record.mp4
    echo ========================================
) else (
    echo.
    echo ========================================
    echo   Error while compiling video!
    echo ========================================
)

echo.
pause
