@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-p5b1-terrain-water
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-p5b1-terrain-water-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-p5b1
if errorlevel 1 goto :fail
echo P5b.1 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo P5b.1 certification FAILED.
popd
exit /b 1
