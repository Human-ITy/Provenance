@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
"Build\x64_Release\ProvenanceClient.exe" --play-mw3-regional-erosion
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
