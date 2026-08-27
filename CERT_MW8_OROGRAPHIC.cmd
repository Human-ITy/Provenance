@echo off
setlocal
pushd "%~dp0"
if not exist "Data\Worldgen\orographic_phase17\canonical_orographic_page_1_1.json" (
  python Tools\Worldgen\export_orographic_pages.py
  if errorlevel 1 (
    echo orographic page export FAILED.
    popd
    exit /b 1
  )
)
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw8-orographic
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
