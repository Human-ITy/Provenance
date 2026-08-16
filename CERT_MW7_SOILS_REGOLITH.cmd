@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw7-soils-regolith
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw7-soils-regolith-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-mw7
if errorlevel 1 goto :fail
echo MW7 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo MW7 certification FAILED.
popd
exit /b 1
