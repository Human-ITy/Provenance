@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  pause
  popd
  exit /b 1
)
start "" "Build\x64_Release\ProvenanceClient.exe" --play-stage11-fault
popd
