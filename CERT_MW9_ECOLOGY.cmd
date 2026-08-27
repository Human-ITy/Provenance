@echo off
setlocal
pushd "%~dp0"
set "CLIENT_ROOT=%CD%"
set "ENGINE=%~dp0..\.ei3qb-cert-workspace\Engine\FableScript\fablescript"
if not exist "%ENGINE%\tools\cert_stage0_live_orographic_stream.py" (
  echo live orographic.phase17 producer missing in FableScript authority.
  popd
  exit /b 1
)
if not exist "Data\Worldgen\orographic_phase17\canonical_orographic_page_1_1.json" (
  echo banked orographic.phase17 oracle missing.
  popd
  exit /b 1
)
if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)
set "PYTHON="
if exist "%LOCALAPPDATA%\Python\bin\python.exe" set "PYTHON=%LOCALAPPDATA%\Python\bin\python.exe"
if not defined PYTHON if exist "%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" set "PYTHON=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if not defined PYTHON set "PYTHON=python"
set "STAGE0_OROGRAPHIC_BANKED=%CLIENT_ROOT%\Data\Worldgen\orographic_phase17"
set "STAGE0_OROGRAPHIC_LIVE_OUT=%CLIENT_ROOT%\Data\Worldgen\orographic_phase17\live"
echo Live-stream emit (orographic_production_page) with banked golden as oracle...
pushd "%ENGINE%"
"%PYTHON%" -m tools.cert_stage0_live_orographic_stream
set EMIT=%ERRORLEVEL%
popd
if not "%EMIT%"=="0" (
  echo LIVE EMIT HOLD
  popd
  exit /b %EMIT%
)
echo MW9 flora consume against live-emitted pages...
start /wait "" "Build\x64_Release\ProvenanceClient.exe" --cert-mw9-ecology
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
