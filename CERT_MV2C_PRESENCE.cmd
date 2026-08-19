@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV2.C distant-terrain PRESENCE certificate (presentation only). Improves how the
rem existing MV2 authority READS at distance without changing the authority: a gentler
rem 32-128 km aerial tail (so far silhouettes keep mass), a coarse land-cover palette
rem (vegetated / substrate / exposed-rock by elevation+slope; bare high ground is rock,
rem NEVER called snow), and stronger silhouette-preserving hillshade. No change to the
rem macro field, amplitudes, region cadence, or 1 km sample step.
rem
rem A/B (run the MV2.B baseline first for the comparison):
rem   CERT_MV2B_HORIZON_RENDERER.cmd   (--mv2c-off baseline; far_separation_from_sky ~0.007)
rem   CERT_MV2C_PRESENCE.cmd           (MV2.C; far_separation_from_sky ~0.049, fixture 13 PASS)
rem Distance-class pixels are depth-based and identical between the two; MV2.C only
rem changes colour/attenuation, never geometry or coverage. --mv2c-off == frozen MV2.B.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv2c-presence
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv2c_presence_cert.txt" type "Docs\provenance_mv2c_presence_cert.txt"
popd
exit /b %RESULT%
