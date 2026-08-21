@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Integration\Start-Ei3CanonicalPlayable.ps1"
set "EI3_EXIT=%ERRORLEVEL%"
if not "%EI3_EXIT%"=="0" (
  echo.
  echo Canonical playable launch failed with code %EI3_EXIT%.
  pause
)
exit /b %EI3_EXIT%
