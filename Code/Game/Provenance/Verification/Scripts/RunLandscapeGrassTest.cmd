@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
pushd "%~dp0..\Artifacts\Landscape"
cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /Fe:LandscapeGrassTest.exe /Fo:LandscapeGrassTest.obj "%~dp0..\LandscapeGrassTest.cpp"
if errorlevel 1 (popd & exit /b 2)
LandscapeGrassTest.exe
set "GRASS_RESULT=%errorlevel%"
popd
exit /b %GRASS_RESULT%
