@echo off
REM Provenance client launcher.
REM Expects the Python voxel bridge already listening on 127.0.0.1:8765
REM   cd <Mygame>\_engine_truth_lane3\fablescript
REM   PYTHONPATH=. python voxel_bridge.py

set ROOT=%~dp0
set EXE=
REM Pick the newest ProvenanceClient.exe under Build\x64_Release* (LNK1104 spill folders included).
for /f "delims=" %%F in ('powershell -NoProfile -Command "$c=Get-ChildItem -Path '%ROOT%Build' -Directory -Filter 'x64_Release*' -ErrorAction SilentlyContinue | ForEach-Object { $p=Join-Path $_.FullName 'ProvenanceClient.exe'; if(Test-Path $p){ Get-Item $p } }; if($c){ ($c | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }"') do set EXE=%%F

if not defined EXE (
  echo ProvenanceClient.exe missing. Build Esoterica.Applications.ProvenanceClient Release^|x64 first.
  exit /b 1
)
echo Launching %EXE% %*
REM Extra args forwarded (e.g. --geo-fixture=torture). Default fixture = range.
start "" "%EXE%" 127.0.0.1 8765 %*
