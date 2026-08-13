@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
"Build\x64_Release\ProvenanceClient.exe" --cert-stage16-dry-hydrology
if errorlevel 1 goto :fail
"Build\x64_Release\ProvenanceClient.exe" --cert-stage16-dry-hydrology-visual
if errorlevel 1 goto :fail
"Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-stage16
if errorlevel 1 goto :fail
echo Stage 16A analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo Stage 16A certification FAILED.
popd
exit /b 1
