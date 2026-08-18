@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem PX1 - Baseline Present-Pacing Stability. Instrumentation + attribution only.
rem Runs the MV1-OFF frozen MW8 baseline (so PX1 cannot blame far terrain) and
rem separates, per travel frame: inter-frame gap (pump/OS/pacing), engine CPU
rem frame production, GPU glFinish, SwapBuffers, engine_work, and the presented
rem swap-to-swap cadence the player actually receives. Diagnostic toggles:
rem   --px1-swap-interval=0^|1   (vsync off/on; restore production afterwards)
rem   --soak-no-glfinish        (drop the protective pre-present completion probe)
"Build\x64_Release\ProvenanceClient.exe" --cert-px1-present-pacing
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_px1_present_pacing_cert.txt" type "Docs\provenance_px1_present_pacing_cert.txt"
popd
exit /b %RESULT%
