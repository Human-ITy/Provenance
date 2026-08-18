@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV1.D - distance/depth readability via continuous aerial perspective.
rem Presentation-only per-fragment atmospheric transmittance toward the horizon
rem sky colour, exp(-(rho*d)^2), by ACTUAL camera-to-surface distance in the far
rem pass only (the frozen 0.03-600 m near pass stays unfogged, so MV1.C geometry,
rem depth split and VBO ownership are byte-identical). Proof: analytical
rem monotone/continuous transmittance with no discontinuity at the LOD band edges
rem (192 m / 2 km / 20 km); and, over the five frozen stations, contrast falls and
rem atmospheric blend rises by physical distance while regional/horizon bands stay
rem rasterised. Station images kept as the visual contact sheet.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv1d-distance-readability
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv1d_distance_readability_cert.txt" type "Docs\provenance_mv1d_distance_readability_cert.txt"
popd
exit /b %RESULT%
