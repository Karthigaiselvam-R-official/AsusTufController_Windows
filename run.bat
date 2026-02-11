@echo off
cd /d "%~dp0"

:: Check for Admin privileges
NET SESSION >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo Requesting administrative privileges...
    powershell -Command "Start-Process '%~dp0run.bat' -Verb RunAs"
    exit
)

:: Launch as SYSTEM using PsExec (Non-interactive -d flag closes terminal)
if not exist "%~dp0PsExec.exe" (
    echo Error: PsExec.exe not found in %~dp0
    pause
    exit
)

"%~dp0PsExec.exe" -i -s -d "%~dp0AsusTufController_Windows.exe"
exit
