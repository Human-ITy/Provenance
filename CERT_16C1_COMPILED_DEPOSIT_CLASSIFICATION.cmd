@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-16c1-compiled-deposit-classification
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-16c1-compiled-deposit-classification-visual
if errorlevel 1 goto :fail
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-16c1
if errorlevel 1 goto :fail
echo Stage 16C.1 analytical, visual, and cardinal certificates PASS.
popd
exit /b 0
:fail
echo Stage 16C.1 certification FAILED.
popd
exit /b 1
