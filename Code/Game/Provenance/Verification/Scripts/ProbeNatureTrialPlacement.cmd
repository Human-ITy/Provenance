@echo off
setlocal
set "TRIAL_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MovementBaseline"
pushd "%TRIAL_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:NaturePlacement.exe /Fo:NaturePlacement.obj "%~dp0..\NatureTrialPlacementTest.cpp"
if errorlevel 1 exit /b 2
NaturePlacement.exe > nature-placement.json
set "RESULT=%errorlevel%"
type nature-placement.json
popd
exit /b %RESULT%
