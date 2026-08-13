@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

"Build\x64_Release\ProvenanceClient.exe" --cert-stage11-shift-scaling
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_stage11_shift_scaling.txt" type "Docs\provenance_stage11_shift_scaling.txt"
popd
exit /b %RESULT%
