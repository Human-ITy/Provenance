@echo off
setlocal
pushd "%~dp0"

set EXE=
for /f "delims=" %%F in ('powershell -NoProfile -Command "$c=Get-ChildItem -Path '%~dp0Build' -Directory -Filter 'x64_Release*' -ErrorAction SilentlyContinue | ForEach-Object { $p=Join-Path $_.FullName 'ProvenanceClient.exe'; if(Test-Path $p){ Get-Item $p } }; if($c){ ($c | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }"') do set EXE=%%F

if not defined EXE (
    echo A Release build of ProvenanceClient was not found.
    echo.
    echo Build ProvenanceClient in Release x64 first.
    pause
    popd
    exit /b 1
)

start "Provenance Worldgen - Latest Certified Runtime" "%EXE%" --play-worldgen-baseline --live-radius=192 --far-extent=0
popd
endlocal
