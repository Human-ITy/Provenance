@echo off
setlocal
for %%I in ("%~dp0..\..\..\..\..") do set "LOG_ROOT=%%~fI"
set "LOG_OUTPUT=%LOG_ROOT%\Build\Verification\GrassWind"
if not exist "%LOG_OUTPUT%" mkdir "%LOG_OUTPUT%"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /W4 /WX /EHsc "%LOG_ROOT%\Code\Game\Provenance\Verification\LogAssertDllSmokeTest.cpp" /Fo"%LOG_OUTPUT%\LogAssertDllSmokeTest.obj" /Fe"%LOG_OUTPUT%\LogAssertDllSmokeTest.exe" /link "%LOG_ROOT%\Build\x64_Debug\Esoterica.Base.lib"
if errorlevel 1 exit /b %errorlevel%
set "PATH=%LOG_ROOT%\Build\x64_Debug;%PATH%"
"%LOG_OUTPUT%\LogAssertDllSmokeTest.exe"
exit /b %errorlevel%
