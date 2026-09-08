@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "OUT=%~dp0..\..\output\integration\starter_tools_v001"
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:"%OUT%\PlacementProbe.exe" /Fo:"%OUT%\PlacementProbe.obj" "%~dp0..\StarterToolsPlacementProbe.cpp"
if errorlevel 1 exit /b 1
"%OUT%\PlacementProbe.exe" > "%OUT%\placement_support.json"
exit /b %errorlevel%
