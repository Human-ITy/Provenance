@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "OUT=%~dp0..\..\output\models\tools\hammer\v001"
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:"%OUT%\ToolPickupTest.exe" /Fo:"%OUT%\ToolPickupTest.obj" "%~dp0..\ToolPickupTest.cpp"
if errorlevel 1 exit /b 1
"%OUT%\ToolPickupTest.exe"
exit /b %errorlevel%
