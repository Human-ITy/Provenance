@echo off
setlocal
for %%I in ("%~dp0..\..\..\..\..") do set "LAYOUT_ROOT=%%~fI"
set "LAYOUT_OUTPUT=%LAYOUT_ROOT%\Build\Verification\GrassWind"
node "%~dp0GenerateShaderParameterLayoutCases.cjs"
if errorlevel 1 exit /b %errorlevel%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /W4 /WX /EHsc /I"%LAYOUT_ROOT%\Code" /I"%LAYOUT_OUTPUT%" "%LAYOUT_ROOT%\Code\Game\Provenance\Verification\ShaderParameterLayoutTest.cpp" /Fo"%LAYOUT_OUTPUT%\ShaderParameterLayoutTest.obj" /Fe"%LAYOUT_OUTPUT%\ShaderParameterLayoutTest.exe"
if errorlevel 1 exit /b %errorlevel%
"%LAYOUT_OUTPUT%\ShaderParameterLayoutTest.exe"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /W4 /WX /EHsc /I"%LAYOUT_OUTPUT%" "%LAYOUT_ROOT%\Code\Game\Provenance\Verification\LogAssertFormatTest.cpp" /Fo"%LAYOUT_OUTPUT%\LogAssertFormatTest.obj" /Fe"%LAYOUT_OUTPUT%\LogAssertFormatTest.exe"
if errorlevel 1 exit /b %errorlevel%
"%LAYOUT_OUTPUT%\LogAssertFormatTest.exe"
exit /b %errorlevel%
