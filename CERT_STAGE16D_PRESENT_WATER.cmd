@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-stage16d-present-water
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-stage16d-present-water-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-stage16d
if errorlevel 1 goto :fail
echo Stage 16D analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo Stage 16D certification FAILED.
popd
exit /b 1
