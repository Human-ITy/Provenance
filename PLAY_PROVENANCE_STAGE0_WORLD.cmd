@echo off
setlocal
set "CANONICAL_ROOT=%~dp0..\.."
if not exist "%CANONICAL_ROOT%\CANONICAL_WORKSPACE.json" set "CANONICAL_ROOT=%~dp0..\.ei3qb-cert-workspace"
powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Tools\Integration\Start-ProvenanceWorldLibrary.ps1" -CanonicalWorkspaceRoot "%CANONICAL_ROOT%"
if errorlevel 1 pause
endlocal
