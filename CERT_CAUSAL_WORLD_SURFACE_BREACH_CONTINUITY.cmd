@echo off
setlocal
cd /d "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo Missing Build\x64_Release\ProvenanceClient.exe
  exit /b 2
)
"Build\x64_Release\ProvenanceClient.exe" --cert-causal-world-surface-breach-continuity
exit /b %ERRORLEVEL%
