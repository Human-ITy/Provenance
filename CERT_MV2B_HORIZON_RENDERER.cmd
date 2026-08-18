@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV2.B horizon renderer certificate. Six viewpoints (the five frozen MV1
rem stations + one ground-level vista aimed across multiple macro pages). Proves,
rem from the composited framebuffer:
rem   1  real 128 km raster    - nonzero pixels in the 32-50/50-80/80-100/100-128 km
rem                              distance classes (depth-binned; not merely submitted)
rem   2  non-repetition        - distinct neighbouring macro pages, distinct digests
rem   3  32 km seam continuous - macro vs frozen MV1 ReconstructedZ < 25 m at 32 km
rem   9  no fine authority     - horizon built from cheap macro pages, no ReconstructedZ
rem  10  bounded residency     - pages bounded by the ring, no travel-history growth
rem  11  no MV2 coverage hole  - a terrain-present mask (macro+MV1+near) plus an
rem                              authority raymarch: zero >=32 km sky pixels are holes
rem                              (sub-32 km LOD slivers are frozen MV1's own domain)
rem   8  off == frozen         - MV2.B default off; MV1.C/D/G reproduce byte-for-byte
rem The cert-colour gap-classification maps and station images are kept as evidence.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv2b-horizon
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv2b_horizon_cert.txt" type "Docs\provenance_mv2b_horizon_cert.txt"
popd
exit /b %RESULT%
