@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

REM EVERY CUT: cardinal replacement on the latest play stage (P5b.2A).
REM Walk / sprint / fly, N/E/S/W, 192 m completeness, 0 movement frames over 16.667 ms.
REM Do not treat this as a soak. Long-haul travel is CERT_STREAMING_SOAK.cmd.
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-worldgen-cardinal-replacement-p5b2a
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_p5b2a_cardinal_replacement_cert.txt" type "Docs\provenance_p5b2a_cardinal_replacement_cert.txt"
popd
exit /b %RESULT%
