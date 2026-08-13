@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (popd & exit /b 1)
"Build\x64_Release\ProvenanceClient.exe" --cert-stage14-single-pick-visual
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_stage14_single_pick_visual_cert.txt" type "Docs\provenance_stage14_single_pick_visual_cert.txt"
popd
exit /b %RESULT%
