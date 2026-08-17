@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV1 multi-scale terrain visibility: teleports through five fixed
rem absolute-coordinate player stations (trunk valley, mountain flank, ridge
rem shoulder, foreland basin, TRUE high divide), settles each band, captures a
rem player-view .ppm per station, and evaluates the six hard fixtures + the
rem geometric-fidelity fixture + horizon-to-hand identity.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv1-multi-scale-terrain
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv1_multi_scale_terrain_cert.txt" type "Docs\provenance_mv1_multi_scale_terrain_cert.txt"
popd
exit /b %RESULT%
