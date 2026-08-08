@echo off
REM Phase 4 Provenance client (handful interaction digests + 6ft walk + far heightfield).
REM Expects the Python voxel bridge already listening on 127.0.0.1:8765
REM   cd <Mygame>\_engine_truth_lane3\fablescript
REM   PYTHONPATH=. python voxel_bridge.py

set ROOT=%~dp0
set BIN=%ROOT%Build\x64_Release
REM Fresh link lands here when the running client locks the primary exe (LNK1104).
if exist "%ROOT%Build\x64_Release_new\ProvenanceClient.exe" (
  set BIN=%ROOT%Build\x64_Release_new
)
if not exist "%BIN%\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Esoterica.Applications.ProvenanceClient Release^|x64 first.
  exit /b 1
)
echo Launching %BIN%\ProvenanceClient.exe
start "" "%BIN%\ProvenanceClient.exe" 127.0.0.1 8765
