@echo off
setlocal
set "HOTSPOT_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MovementBaseline"
if not exist "%HOTSPOT_OUTPUT%" mkdir "%HOTSPOT_OUTPUT%"
pushd "%HOTSPOT_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (popd & exit /b 2)
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:MovementHotspot.exe /Fo:MovementHotspot.obj "%~dp0..\GraniteMovementHotspotDiagnostic.cpp"
if errorlevel 1 (popd & exit /b 2)
MovementHotspot.exe > hotspot-optimized.txt
set "HOTSPOT_OPT=%errorlevel%"
type hotspot-optimized.txt
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:MovementHotspotDebug.exe /Fo:MovementHotspotDebug.obj "%~dp0..\GraniteMovementHotspotDiagnostic.cpp"
if errorlevel 1 (popd & exit /b 2)
MovementHotspotDebug.exe > hotspot-debug.txt
set "HOTSPOT_DEBUG=%errorlevel%"
type hotspot-debug.txt
popd
if not "%HOTSPOT_OPT%"=="0" exit /b 1
if not "%HOTSPOT_DEBUG%"=="0" exit /b 1
exit /b 0
