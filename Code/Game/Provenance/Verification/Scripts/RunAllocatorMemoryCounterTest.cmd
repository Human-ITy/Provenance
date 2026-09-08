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
cl /nologo /O2 /DENABLE_STATISTICS=1 /Fo:AllocatorMemoryCounterRpmalloc.obj /c ..\..\Code\Base\ThirdParty\rpmalloc\rpmalloc.c
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /O2 /Fe:AllocatorMemoryCounterTest.exe /Fo:AllocatorMemoryCounterTest.obj ..\..\Code\Game\Provenance\Verification\AllocatorMemoryCounterTest.cpp AllocatorMemoryCounterRpmalloc.obj Advapi32.lib
if errorlevel 1 exit /b %errorlevel%
AllocatorMemoryCounterTest.exe
exit /b %errorlevel%
