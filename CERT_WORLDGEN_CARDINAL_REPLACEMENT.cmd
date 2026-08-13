@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

"Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement --live-radius=192 --far-extent=0
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_worldgen_cardinal_replacement_cert.txt" type "Docs\provenance_worldgen_cardinal_replacement_cert.txt"
popd
exit /b %RESULT%
