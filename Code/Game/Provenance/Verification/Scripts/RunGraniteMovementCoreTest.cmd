@echo off
setlocal
set "CORE_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MovementBaseline"
if not exist "%CORE_OUTPUT%" mkdir "%CORE_OUTPUT%"
pushd "%CORE_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:MovementCore.exe /Fo:MovementCore.obj "%~dp0..\GraniteMovementCoreTest.cpp"
if errorlevel 1 exit /b 2
MovementCore.exe
set "CORE_OPT=%errorlevel%"
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:MovementCoreDebug.exe /Fo:MovementCoreDebug.obj "%~dp0..\GraniteMovementCoreTest.cpp"
if errorlevel 1 exit /b 2
MovementCoreDebug.exe
set "CORE_DEBUG=%errorlevel%"
popd
if not "%CORE_OPT%"=="0" exit /b 1
if not "%CORE_DEBUG%"=="0" exit /b 1
exit /b 0
