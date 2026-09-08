@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
if not exist "%~dp0..\Artifacts\SupportReuse" mkdir "%~dp0..\Artifacts\SupportReuse"
pushd "%~dp0..\Artifacts\SupportReuse"
cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /Fe:GraniteSupportReuseTest.exe /Fo:GraniteSupportReuseTest.obj "%~dp0..\GraniteSupportReuseTest.cpp"
if errorlevel 1 (popd & exit /b 2)
GraniteSupportReuseTest.exe %*
set "SUPPORT_REUSE_RESULT=%errorlevel%"
popd
exit /b %SUPPORT_REUSE_RESULT%
