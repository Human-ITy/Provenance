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
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:EditorMouseStateRegression.exe "%~dp0..\Fixtures\EditorMouseStateRegression.cpp"
if errorlevel 1 exit /b %errorlevel%
EditorMouseStateRegression.exe
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:EditorMouseStateRegressionOptimized.exe "%~dp0..\Fixtures\EditorMouseStateRegression.cpp"
if errorlevel 1 exit /b %errorlevel%
EditorMouseStateRegressionOptimized.exe
exit /b %errorlevel%
