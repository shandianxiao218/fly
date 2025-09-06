@echo off
chcp 65001 >nul
REM Beidou Navigation Satellite Visibility Analysis System - Web Server Startup Script
REM Windows version startup script

echo Beidou Navigation Satellite Visibility Analysis System - Web Server Startup Script
echo =================================================

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python is not installed
    pause
    exit /b 1
)

REM Check if index.html exists in current directory
if not exist "index.html" (
    echo Error: index.html does not exist in current directory
    echo Please run this script from the web directory
    pause
    exit /b 1
)

REM Set port
set PORT=%1
if "%PORT%"=="" set PORT=8080

echo Starting Web Server...
echo Port: %PORT%
echo Web Root Directory: %CD%
echo Press Ctrl+C to stop the server
echo =================================================

REM Start Python HTTP server
python test_server.py %PORT%

pause