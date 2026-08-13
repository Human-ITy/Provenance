@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (popd & exit /b 1)
start "Provenance Stage 14" "Build\x64_Release\ProvenanceClient.exe" --play-stage14-single-pick
popd
