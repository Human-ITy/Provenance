@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start "" /wait "Build\x64_Release\ProvenanceClient.exe" --cert-stage11-fault-visual
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_stage11_fault_identity_xray.txt" type "Docs\provenance_stage11_fault_identity_xray.txt"
popd
exit /b %RESULT%
