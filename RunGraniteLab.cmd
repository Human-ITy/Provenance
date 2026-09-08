@echo off
setlocal
set "LAB_ROOT=%~dp0"
if not exist "%LAB_ROOT%Build\x64_Debug\EsotericaEditor.exe" (
  echo Missing Debug editor. Open GraniteLab.slnx and build Granite Lab first.
  exit /b 1
)
if not exist "%LAB_ROOT%Data\Provenance\ProvenanceSandbox.map" (
  echo Missing ProvenanceSandbox.map.
  exit /b 1
)
if /i "%~1"=="check" (
  echo Granite Lab editor and sandbox paths exist. No application launched.
  exit /b 0
)
start "" /D "%LAB_ROOT%Build\x64_Debug" "%LAB_ROOT%Build\x64_Debug\EsotericaEditor.exe" -map data://provenance/provenancesandbox.map
exit /b %errorlevel%
