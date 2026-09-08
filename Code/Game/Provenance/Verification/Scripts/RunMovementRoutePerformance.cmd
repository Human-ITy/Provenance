@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
if not exist "%~dp0..\Artifacts\Landscape" mkdir "%~dp0..\Artifacts\Landscape"
pushd "%~dp0..\Artifacts\Landscape"
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:MovementRoutePerformance.exe /Fo:MovementRoutePerformance.obj "%~dp0..\MovementRoutePerformance.cpp"
if errorlevel 1 (popd & exit /b 2)
MovementRoutePerformance.exe > movement-routes-optimized.txt
set "ROUTE_RESULT=%errorlevel%"
type movement-routes-optimized.txt
popd
exit /b %ROUTE_RESULT%
