@echo off
setlocal
set ROOT=%~dp0
set EXE=
for /f "delims=" %%F in ('powershell -NoProfile -Command "$c=Get-ChildItem -Path '%ROOT%Build' -Directory -Filter 'x64_Release*' -ErrorAction SilentlyContinue | ForEach-Object { $p=Join-Path $_.FullName 'ProvenanceClient.exe'; if(Test-Path $p){ Get-Item $p } }; if($c){ ($c | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }"') do set EXE=%%F
if not defined EXE exit /b 1
cd /d "%ROOT%"
"%EXE%" --cert-playable-runtime-independence
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_playable_runtime_independence_cert.txt" type "Docs\provenance_playable_runtime_independence_cert.txt"
if exist "Docs\provenance_playtest_toolkit_surface_attachment_cert.txt" type "Docs\provenance_playtest_toolkit_surface_attachment_cert.txt"
if exist "Docs\provenance_worldgen_boundary_continuity_cert.txt" type "Docs\provenance_worldgen_boundary_continuity_cert.txt"
exit /b %RESULT%
