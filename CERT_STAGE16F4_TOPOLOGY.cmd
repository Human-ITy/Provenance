@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-stage16f4-topology
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-stage16f4-topology-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-stage16f4
if errorlevel 1 goto :fail
echo Stage 16F.4 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo Stage 16F.4 certification FAILED.
popd
exit /b 1
