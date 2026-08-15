@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

REM Test S: teleport+settle semantic-distance. Independent of Test B soak.
REM Does not run 90/300/900. P5b.2C / P5b.3 stay CLOSED.
"Build\x64_Release\ProvenanceClient.exe" --cert-semantic-distance --live-radius=192 --far-extent=0
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_semantic_distance_cert.txt" type "Docs\provenance_semantic_distance_cert.txt"
popd
exit /b %RESULT%
