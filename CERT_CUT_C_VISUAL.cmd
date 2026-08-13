@echo off
setlocal
pushd "%~dp0"
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
start "" /wait "Build\x64_Release\ProvenanceClient.exe" --cert-cut-c-visual
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_cut_c_visual_cert.txt" type "Docs\provenance_cut_c_visual_cert.txt"
if not "%RESULT%"=="0" (
  popd
  exit /b %RESULT%
)
start "" /wait "Build\x64_Release\ProvenanceClient.exe" --cert-cut-c-xray-visual
set RESULT=%ERRORLEVEL%
if exist "Docs\provenance_cut_c_xray_visual_cert.txt" type "Docs\provenance_cut_c_xray_visual_cert.txt"
popd
exit /b %RESULT%
