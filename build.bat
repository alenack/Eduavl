@echo off
echo ===================================================================
echo Building Real-Time Student Record Management System (C AVL Backend)
echo ===================================================================

set GCC="C:\Program Files\CodeBlocks\MinGW\bin\gcc.exe"

if not exist %GCC% (
    echo [ERROR] MinGW GCC not found at %GCC%
    echo Checking PATH for gcc...
    where gcc >nul 2>nul
    if %errorlevel% neq 0 (
        echo [ERROR] No GCC compiler found. Please install MinGW or Code::Blocks.
        pause
        exit /b 1
    )
    set GCC=gcc
)

echo [1/2] Compiling Automated Unit Tests (test_avl.exe)...
%GCC% -Wall -Wextra -O2 avl.c test_avl.c -o test_avl.exe
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_avl.c
    pause
    exit /b 1
)

echo [2/2] Compiling Embedded C Web Server (server.exe)...
%GCC% -Wall -Wextra -O2 avl.c server.c -o server.exe -lws2_32
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile server.c
    pause
    exit /b 1
)

echo.
echo ===================================================================
echo Build Successful!
echo Executables created:
echo   - server.exe   (Winsock HTTP/REST Server + AVL Engine)
echo   - test_avl.exe (Automated Unit Test Suite)
echo ===================================================================
