@echo off
setlocal
pushd "%~dp0"
if not exist "Build\\x64_Release\\ProvenanceClient.exe" (popd & exit /b 1)
"Build\\x64_Release\\ProvenanceClient.exe" --cert-living-world-load=3
set RESULT=%ERRORLEVEL%
if exist "Docs\\provenance_living_world_load_3_cert.txt" type "Docs\\provenance_living_world_load_3_cert.txt"
popd
exit /b %RESULT%
