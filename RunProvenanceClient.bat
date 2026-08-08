@echo off
REM Provenance client launcher.
REM Expects the Python voxel bridge already listening on 127.0.0.1:8765
REM   cd <Mygame>\_engine_truth_lane3\fablescript
REM   PYTHONPATH=. python voxel_bridge.py

set ROOT=%~dp0
set EXE=
REM Pick the newest ProvenanceClient.exe (x64_Release_new is only a LNK1104 spill —
REM never permanently shadow a fresher primary build).
for /f "delims=" %%F in ('powershell -NoProfile -Command "$c=@(); foreach($p in @('%ROOT%Build\x64_Release\ProvenanceClient.exe','%ROOT%Build\x64_Release_new\ProvenanceClient.exe')){ if(Test-Path $p){ $c += Get-Item $p } }; if($c.Count -gt 0){ ($c | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }"') do set EXE=%%F

if not defined EXE (
  echo ProvenanceClient.exe missing. Build Esoterica.Applications.ProvenanceClient Release^|x64 first.
  exit /b 1
)
echo Launching %EXE%
start "" "%EXE%" 127.0.0.1 8765
