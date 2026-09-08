@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
if not exist "%~dp0..\Artifacts\Landscape" mkdir "%~dp0..\Artifacts\Landscape"
pushd "%~dp0..\Artifacts\Landscape"
cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /Fe:TerrainResponseBaseline.exe /Fo:TerrainResponseBaseline.obj "%~dp0..\TerrainResponseBaseline.cpp"
if errorlevel 1 (popd & exit /b 2)
TerrainResponseBaseline.exe
set "RESPONSE_RESULT=%errorlevel%"
popd
exit /b %RESPONSE_RESULT%
