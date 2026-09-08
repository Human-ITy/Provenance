@echo off
setlocal
set "TREAD_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MovementBaseline"
if not exist "%TREAD_OUTPUT%" mkdir "%TREAD_OUTPUT%"
pushd "%TREAD_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:TreadSurfaceTest.exe /Fo:TreadSurfaceTest.obj "%~dp0..\GraniteTreadSurfaceTest.cpp"
if errorlevel 1 exit /b 2
TreadSurfaceTest.exe
set "TREAD_OPT=%errorlevel%"
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:TreadSurfaceTestDebug.exe /Fo:TreadSurfaceTestDebug.obj "%~dp0..\GraniteTreadSurfaceTest.cpp"
if errorlevel 1 exit /b 2
TreadSurfaceTestDebug.exe
set "TREAD_DEBUG=%errorlevel%"
popd
if not "%TREAD_OPT%"=="0" exit /b 1
if not "%TREAD_DEBUG%"=="0" exit /b 1
exit /b 0
