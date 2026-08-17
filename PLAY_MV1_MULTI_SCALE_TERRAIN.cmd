@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem Free-fly the MW8 biome world with MV1 multi-scale terrain visible to 32 km.
rem Near 192 m stays fully interactive; derived meso/regional/horizon bands
rem extend the visible landscape from the same absolute-coordinate MW authority.
rem Add --mv1-off to compare against the MW8 near-only baseline.
"Build\x64_Release\ProvenanceClient.exe" --play-mv1-multi-scale-terrain %*
popd
