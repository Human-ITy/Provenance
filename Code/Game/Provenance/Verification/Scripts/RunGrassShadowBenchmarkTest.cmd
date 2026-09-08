@echo off
setlocal
for %%I in ("%~dp0..\..\..\..\..") do set "SHADOW_ROOT=%%~fI"
set "SHADOW_OUTPUT=%SHADOW_ROOT%\Build\Verification\GrassShadows"
if not exist "%SHADOW_OUTPUT%" mkdir "%SHADOW_OUTPUT%"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /W4 /WX /EHsc /I"%SHADOW_ROOT%\Code" "%SHADOW_ROOT%\Code\Game\Provenance\Verification\GrassShadowBenchmarkTest.cpp" /Fo"%SHADOW_OUTPUT%\GrassShadowBenchmarkTest.obj" /Fe"%SHADOW_OUTPUT%\GrassShadowBenchmarkTest.exe"
if errorlevel 1 exit /b %errorlevel%
pushd "%SHADOW_OUTPUT%"
GrassShadowBenchmarkTest.exe
set "SHADOW_RESULT=%errorlevel%"
popd
exit /b %SHADOW_RESULT%
