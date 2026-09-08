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
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:SoilScoopTest.exe /Fo:SoilScoopTest.obj ..\..\Code\Game\Provenance\Verification\SoilScoopTest.cpp
if errorlevel 1 exit /b %errorlevel%
SoilScoopTest.exe
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:SoilScoopTestDebug.exe /Fo:SoilScoopTestDebug.obj ..\..\Code\Game\Provenance\Verification\SoilScoopTest.cpp
if errorlevel 1 exit /b %errorlevel%
SoilScoopTestDebug.exe
exit /b %errorlevel%
