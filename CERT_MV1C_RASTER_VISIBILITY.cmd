@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV1.C - real 32 km raster visibility for the frozen MV1 geography.
rem Two-pass depth split: a wide 128 m-33 km far pass rasterizes the MV1
rem meso/regional/horizon bands, a depth-only clear hands a clean depth buffer
rem to the frozen 0.03-600 m near pass, which paints the 192 m authoritative
rem world over the top. Proof is per-band framebuffer pixel binning by real
rem camera distance (submitted-but-clipped geometry paints zero pixels), across
rem the five frozen player stations, with the station images kept as evidence.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv1c-raster-visibility
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv1c_raster_visibility_cert.txt" type "Docs\provenance_mv1c_raster_visibility_cert.txt"
popd
exit /b %RESULT%
