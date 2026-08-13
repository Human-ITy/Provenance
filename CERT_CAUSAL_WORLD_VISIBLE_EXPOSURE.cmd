@echo off
setlocal
set ROOT=%~dp0
set EXE=
for /f "delims=" %%F in ('powershell -NoProfile -Command "$c=Get-ChildItem -Path '%ROOT%Build' -Directory -Filter 'x64_Release*' -ErrorAction SilentlyContinue | ForEach-Object { $p=Join-Path $_.FullName 'ProvenanceClient.exe'; if(Test-Path $p){ Get-Item $p } }; if($c){ ($c | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }"') do set EXE=%%F
if not defined EXE (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  exit /b 1
)
cd /d "%ROOT%"
"%EXE%" --cert-causal-world-visible-exposure
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_causal_world_visible_geologic_exposure_cert.txt" type "Docs\provenance_causal_world_visible_geologic_exposure_cert.txt"
exit /b %RESULT%
