@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw3-regional-erosion
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw3-regional-erosion-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-mw3
if errorlevel 1 goto :fail
echo MW3 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo MW3 certification FAILED.
popd
exit /b 1
