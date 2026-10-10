@echo off
echo ===================================================================
echo Launching Real-Time Student Record Management System
echo ===================================================================

call build.bat
if %errorlevel% neq 0 (
    echo [ERROR] Build failed. Exiting.
    pause
    exit /b 1
)

echo.
echo Starting Web Dashboard on http://localhost:8080 ...
start http://localhost:8080/

echo Starting C AVL Backend Server...
server.exe 8080
