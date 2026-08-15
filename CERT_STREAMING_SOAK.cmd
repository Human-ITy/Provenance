@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

REM First-landing soak: 90 s wall-clock, same metrics as the 5-15 min milestone.
REM Milestone / release: add --soak-duration-s=300 or --soak-duration-s=900
REM 480 m/s certified stress: --soak-speed-mps=480
REM 960 m/s informational only: --soak-speed-mps=960
"Build\x64_Release\ProvenanceClient.exe" --cert-streaming-soak-p5b2a --soak-duration-s=90 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_p5b2a_streaming_soak_cert.txt" type "Docs\provenance_p5b2a_streaming_soak_cert.txt"
popd
exit /b %RESULT%
