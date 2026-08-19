@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV1 live-terrain coverage certificate. Fixed player position/elevation, sweep the
rem camera 360 degrees in yaw. Proves the coverage invariants that the player free-fly
rem screenshots violated:
rem   resident_set_delta_under_yaw = 0    (residency is NOT tied to view direction)
rem   resident/authority digest    stable (no rebuild/authority change from yaw)
rem   below_terrain_sky_holes      = 0    (no sky enclosed by the live terrain surface)
rem The band-boundary LOD-ring underlap that caused blue surface holes + exposed skirts
rem is fixed by an AABB-overlaps-annulus desired-set (see MV1_COVERAGE_HOLE_FIX_HANDOFF).
rem Attribution A/B: add --mv1-nocull to confirm the horizontal frustum cull is NOT the
rem owner (identical holes cull on/off).
"Build\x64_Release\ProvenanceClient.exe" --cert-mv1-coverage
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv1_rotation_cull.txt" type "Docs\provenance_mv1_rotation_cull.txt"
popd
exit /b %RESULT%
