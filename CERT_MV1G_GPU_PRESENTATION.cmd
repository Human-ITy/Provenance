@echo off
setlocal
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem MV1.G — GPU presentation certification for the frozen 32 km MV1 renderer.
rem Async ARB_timer_query (GL_TIME_ELAPSED) around the MV1 draw span, read back
rem after frame latency (never blocks the pipeline to measure). Drives the six
rem scenario classes: five stations, worst-orientation sweep, full 360 rotation,
rem movement band churn, first-visible cold, and warm repeat. Unsupported metrics
rem (VRAM, display-list upload timing) report UNAVAILABLE rather than inventing.
"Build\x64_Release\ProvenanceClient.exe" --cert-mv1g-gpu-presentation
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_mv1g_gpu_presentation_cert.txt" type "Docs\provenance_mv1g_gpu_presentation_cert.txt"
popd
exit /b %RESULT%
