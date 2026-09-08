@echo off
setlocal
set "PLACEMENT_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\Placement"
if not exist "%PLACEMENT_OUTPUT%" mkdir "%PLACEMENT_OUTPUT%"
pushd "%PLACEMENT_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:GranitePlacementTest.exe /Fo:GranitePlacementTest.obj "%~dp0..\GraniteLabPlacementTest.cpp"
if errorlevel 1 exit /b 2
GranitePlacementTest.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:GranitePlacementTestDebug.exe /Fo:GranitePlacementTestDebug.obj "%~dp0..\GraniteLabPlacementTest.cpp"
if errorlevel 1 exit /b 2
GranitePlacementTestDebug.exe
if errorlevel 1 exit /b 1
popd
exit /b 0
