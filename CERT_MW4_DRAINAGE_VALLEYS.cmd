@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw4-drainage-valleys
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw4-drainage-valleys-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-mw4
if errorlevel 1 goto :fail
echo MW4 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo MW4 certification FAILED.
popd
exit /b 1
