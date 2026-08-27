@echo off
setlocal
pushd "%~dp0"
if not exist "Data\Worldgen\orographic_phase17\canonical_orographic_page_1_1.json" (
  echo banked orographic.phase17 page missing.
  popd
  exit /b 1
)
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-stage0-adopt-page
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
