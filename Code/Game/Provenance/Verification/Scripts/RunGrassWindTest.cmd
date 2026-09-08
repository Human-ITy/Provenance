@echo off
setlocal
for %%I in ("%~dp0..\..\..\..\..") do set "WIND_ROOT=%%~fI"
set "WIND_OUTPUT=%WIND_ROOT%\Build\Verification\GrassWind"
if not exist "%WIND_OUTPUT%" mkdir "%WIND_OUTPUT%"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /W4 /WX /EHsc /I"%WIND_ROOT%\Code" "%WIND_ROOT%\Code\Game\Provenance\Verification\GrassWindTest.cpp" /Fo"%WIND_OUTPUT%\GrassWindTest.obj" /Fe"%WIND_OUTPUT%\GrassWindTest.exe"
if errorlevel 1 exit /b %errorlevel%
"%WIND_OUTPUT%\GrassWindTest.exe"
exit /b %errorlevel%
