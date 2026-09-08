@echo off
setlocal
set "ANCHOR_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MaterialAnchor"
if not exist "%ANCHOR_OUTPUT%" mkdir "%ANCHOR_OUTPUT%"
pushd "%ANCHOR_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:GraniteMaterialAnchorTest.exe /Fo:GraniteMaterialAnchorTest.obj "%~dp0..\GraniteMaterialAnchorTest.cpp"
if errorlevel 1 exit /b 2
GraniteMaterialAnchorTest.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:GraniteMaterialAnchorTestDebug.exe /Fo:GraniteMaterialAnchorTestDebug.obj "%~dp0..\GraniteMaterialAnchorTest.cpp"
if errorlevel 1 exit /b 2
GraniteMaterialAnchorTestDebug.exe
if errorlevel 1 exit /b 1
popd
exit /b 0
