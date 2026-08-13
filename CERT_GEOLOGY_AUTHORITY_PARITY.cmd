@echo off
setlocal
pushd "%~dp0"

set PYTHON=C:\Users\D-Day\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe
if not exist "%PYTHON%" set PYTHON=python

"%PYTHON%" Tools\Worldgen\compile_geology_authority_bridge.py
if errorlevel 1 (
  echo FableScript authority bridge compilation failed.
  popd
  exit /b 1
)

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

"Build\x64_Release\ProvenanceClient.exe" --cert-geology-authority-parity
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_fablescript_esoterica_geology_authority_parity_cert.txt" type "Docs\provenance_fablescript_esoterica_geology_authority_parity_cert.txt"
popd
exit /b %RESULT%
