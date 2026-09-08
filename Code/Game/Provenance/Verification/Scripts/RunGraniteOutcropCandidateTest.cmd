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
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /W4 /O2 /I. /DGRANITE_OUTCROP_AUTHORED_SHAPE_HEADER=\"GraniteOutcropCandidate.h\" /Fe:GraniteOutcropCandidateTest.exe /Fo:GraniteOutcropCandidateTest.obj ..\..\Code\Game\Provenance\Verification\GraniteOutcropV2Test.cpp
if errorlevel 1 exit /b %errorlevel%
GraniteOutcropCandidateTest.exe
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /W4 /Od /I. /DGRANITE_OUTCROP_AUTHORED_SHAPE_HEADER=\"GraniteOutcropCandidate.h\" /Fe:GraniteOutcropCandidateTestDebug.exe /Fo:GraniteOutcropCandidateTestDebug.obj ..\..\Code\Game\Provenance\Verification\GraniteOutcropV2Test.cpp
if errorlevel 1 exit /b %errorlevel%
GraniteOutcropCandidateTestDebug.exe
exit /b %errorlevel%
