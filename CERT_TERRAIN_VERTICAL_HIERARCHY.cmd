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
echo Headless numbers + hillshade...
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-terrain-vertical-hierarchy-headless
set NUM=%ERRORLEVEL%
echo Player-view screenshots (ground + free-flight)...
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-terrain-vertical-hierarchy --play-orographic-phase17
set VIS=%ERRORLEVEL%
popd
if not "%NUM%"=="0" exit /b %NUM%
exit /b %VIS%
