@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-p5b3b2-loose-matter-settling
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-p5b3b2-loose-matter-settling-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-p5b3b2
if errorlevel 1 goto :fail
echo P5b.3B.2 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo P5b.3B.2 certification FAILED.
popd
exit /b 1
