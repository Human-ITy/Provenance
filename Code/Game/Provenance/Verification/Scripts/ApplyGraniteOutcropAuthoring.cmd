@echo off
setlocal
rem Resolve from this maintained script; generated outputs stay in the existing build directory.
pushd "%~dp0..\..\..\..\..\Build\x64_Debug"
if errorlevel 1 exit /b 1
call :run %*
set "PROVENANCE_SCRIPT_EXIT=%errorlevel%"
popd
exit /b %PROVENANCE_SCRIPT_EXIT%

:run
set "AUTHORING_OBJ=%~1"
if "%AUTHORING_OBJ%"=="" set "AUTHORING_OBJ=GraniteOutcropAuthoring.obj"
call "%~dp0RunGraniteOutcropShapeTool.cmd" bake "%AUTHORING_OBJ%" GraniteOutcropCandidate.h
if errorlevel 1 exit /b %errorlevel%
call "%~dp0RunGraniteOutcropCandidateTest.cmd"
if errorlevel 1 exit /b %errorlevel%
copy /Y GraniteOutcropCandidate.h ..\..\Code\Game\Provenance\Geometry\GraniteOutcropAuthoredShape.h >nul
if errorlevel 1 exit /b %errorlevel%
echo Authoring surface accepted: exact mass, closed volume, and excavation checks pass.
exit /b %errorlevel%
