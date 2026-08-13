@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

"Build\x64_Release\ProvenanceClient.exe" --cert-stage11-residency-waterfall
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_stage11_residency_waterfall.txt" type "Docs\provenance_stage11_residency_waterfall.txt"
popd
exit /b %RESULT%
