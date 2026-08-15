@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

REM First-landing soak: 90 s travel, 60 s stopped, drain, then return-to-origin settle.
REM Milestone / release: add --soak-duration-s=300 or --soak-duration-s=900
REM 480 m/s certified stress: --soak-speed-mps=480
REM 960 m/s informational only: --soak-speed-mps=960
REM Water submit A/B (physics/occupancy unchanged):
REM   --soak-water-backend=persistent   owned VBO/IBO (default)
REM   --soak-water-backend=legacy       client-array control
REM   --soak-draw=no-water              negative control (water submit off)
"Build\x64_Release\ProvenanceClient.exe" --cert-streaming-soak-p5b2b --soak-duration-s=90 --soak-stop-s=60 --soak-return=1 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_p5b2b_streaming_soak_cert.txt" type "Docs\provenance_p5b2b_streaming_soak_cert.txt"
popd
exit /b %RESULT%
