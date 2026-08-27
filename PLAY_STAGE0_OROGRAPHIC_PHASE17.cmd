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
echo Opt-in: admits certified orographic.phase17 page (1,1) into native Stage0.
echo Does not replace PLAY_PROVENANCE_STAGE0_WORLD.cmd, library spawn, or native render.
"Build\x64_Release\ProvenanceClient.exe" --play-orographic-phase17
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
