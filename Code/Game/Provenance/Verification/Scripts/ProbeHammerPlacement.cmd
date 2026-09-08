@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "OUT=%~dp0..\..\output\models\tools\hammer\v001"
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:"%OUT%\HammerPlacementProbe.exe" /Fo:"%OUT%\HammerPlacementProbe.obj" "%~dp0..\HammerPlacementProbe.cpp"
if errorlevel 1 exit /b 1
"%OUT%\HammerPlacementProbe.exe" > "%OUT%\placement.json"
exit /b %errorlevel%
