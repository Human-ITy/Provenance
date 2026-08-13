@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"

if not exist "Build\x64_Release\ProvenanceClient.exe" (
  echo ProvenanceClient.exe missing. Build Release x64 first.
  popd
  exit /b 1
)

for %%S in (4 8 16) do (
  "Build\x64_Release\ProvenanceClient.exe" --cert-stage11-freefly --live-radius=192 --far-extent=0 --freefly-speed=%%S --freefly-distance=512
  if errorlevel 1 (
    set RESULT=!ERRORLEVEL!
    popd
    exit /b !RESULT!
  )
  if exist "Docs\provenance_stage11_freefly_r192_f0_s%%S.txt" type "Docs\provenance_stage11_freefly_r192_f0_s%%S.txt"
)

popd
exit /b 0
