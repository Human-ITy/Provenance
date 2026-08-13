@echo off
setlocal
pushd "%~dp0"

set PYTHON=C:\Users\D-Day\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe
if not exist "%PYTHON%" set PYTHON=python

"%PYTHON%" Tools\Worldgen\compile_cut_c_occupancy_fixture.py
if errorlevel 1 (
  echo Cut-C FableScript occupancy export failed.
  popd
  exit /b 1
)

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

start "" /wait "Build\x64_Release\ProvenanceClient.exe" --cert-cut-c-occupancy-parity
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_cut_c_occupancy_reconstruction_parity_cert.txt" type "Docs\provenance_cut_c_occupancy_reconstruction_parity_cert.txt"
popd
exit /b %RESULT%
