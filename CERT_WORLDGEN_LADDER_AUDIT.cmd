@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

"Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-ladder-audit
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_worldgen_ladder_audit.txt" type "Docs\provenance_worldgen_ladder_audit.txt"
popd
exit /b %RESULT%
