@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV2.B - 100-128 km macro horizon rendered from the MV2.A compiled macro pages
rem (Data\Worldgen\MacroAuthority\*.mcp). Presentation only: the frozen 0-32 km
rem MV1.C/MV1.D world is unchanged; a wide 24-130 km macro pass is drawn behind it,
rem depth-split so MV1 stays authoritative at the 32 km seam. Per-page persistent
rem VBOs (sync-safe). Recalibrated 128 km aerial perspective. --mv2b-off reproduces
rem the frozen MV1 presentation.
"Build\x64_Release\ProvenanceClient.exe" --play-mv2b-horizon %*
popd
