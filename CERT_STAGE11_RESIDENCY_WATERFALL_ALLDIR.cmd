@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

rem The combined receipt is owned by this command. Each direction appends one
rem summary line, so start from an empty file to keep the verdict honest.
if exist "Docs\provenance_stage11_residency_waterfall_alldir.txt" del "Docs\provenance_stage11_residency_waterfall_alldir.txt"

set FAILED=0
for %%D in (north east south west) do (
  echo === capture-free 192 m waterfall: %%D ===
  "Build\x64_Release\ProvenanceClient.exe" --cert-stage11-residency-waterfall-%%D
  if errorlevel 1 set FAILED=1
  if exist "Docs\provenance_stage11_residency_waterfall_%%D.txt" type "Docs\provenance_stage11_residency_waterfall_%%D.txt"
)

if not exist "Docs\provenance_stage11_residency_waterfall_alldir.txt" (
  echo STAGE11_RESIDENCY_WATERFALL_ALLDIR combined receipt missing.
  popd
  exit /b 1
)

echo.
echo === combined all-direction receipt ===
type "Docs\provenance_stage11_residency_waterfall_alldir.txt"

rem The acceptance rule is sustained 60 FPS: walk, sprint and free-fly in every
rem bearing with zero frames above 16.67 ms. Everything else below is recorded
rem as an optimization milestone only and never as a pass.
set PASSED=0
set SUSTAIN=0
set SUB100=0
set SUB50=0
set SUB33=0
set WALKOK=0
set SPRINTOK=0
set FLYOK=0
set ALLDIR=Docs\provenance_stage11_residency_waterfall_alldir.txt
for /f %%A in ('type "!ALLDIR!" ^| find /c "correctness=PASS"') do set PASSED=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "gameplay_60fps=PASS"') do set SUSTAIN=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "walk_over_16=0 "') do set WALKOK=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "sprint_over_16=0 "') do set SPRINTOK=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "fly_over_16=0 "') do set FLYOK=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "over_100=0"') do set SUB100=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "over_50=0"') do set SUB50=%%A
for /f %%A in ('type "!ALLDIR!" ^| find /c "over_33=0"') do set SUB33=%%A

echo.
echo directions_measured=4
echo directions_correct=!PASSED!
echo directions_sustaining_60fps=!SUSTAIN!
echo directions_walk_clean=!WALKOK!
echo directions_sprint_clean=!SPRINTOK!
echo directions_free_fly_clean=!FLYOK!

set GATE=FAIL_SUSTAINED_60_FPS
if "!PASSED!"=="4" if "!SUSTAIN!"=="4" set GATE=PASS_SUSTAINED_60_FPS
echo alldir_performance_gate=!GATE!

set MILESTONE=OPEN
if "!PASSED!"=="4" if "!SUB100!"=="4" set MILESTONE=LT_100_PASS
if "!PASSED!"=="4" if "!SUB50!"=="4" set MILESTONE=LT_50_PASS
if "!PASSED!"=="4" if "!SUB33!"=="4" set MILESTONE=LT_33_3_PASS
echo alldir_optimization_milestone=!MILESTONE!

if not "!PASSED!"=="4" set FAILED=1
popd
exit /b %FAILED%
