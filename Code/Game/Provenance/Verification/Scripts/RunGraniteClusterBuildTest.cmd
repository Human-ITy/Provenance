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
cl /nologo /EHsc /std:c++17 /O2 /I..\..\External\MeshOptimizer\src /c ..\..\Code\Game\Provenance\Verification\GraniteClusterBuildTest.cpp
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /Od /c ..\..\External\MeshOptimizer\src\clusterizer.cpp ..\..\External\MeshOptimizer\src\allocator.cpp ..\..\External\MeshOptimizer\src\vcacheoptimizer.cpp ..\..\External\MeshOptimizer\src\vfetchoptimizer.cpp ..\..\External\MeshOptimizer\src\indexgenerator.cpp
if errorlevel 1 exit /b %errorlevel%
link /nologo /OUT:GraniteClusterBuildTest.exe GraniteClusterBuildTest.obj clusterizer.obj allocator.obj vcacheoptimizer.obj vfetchoptimizer.obj indexgenerator.obj
if errorlevel 1 exit /b %errorlevel%
GraniteClusterBuildTest.exe
exit /b %errorlevel%
