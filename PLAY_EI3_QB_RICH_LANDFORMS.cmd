@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Integration\Start-Ei3CanonicalPlayable.ps1" -RichLandforms
if errorlevel 1 pause
endlocal
