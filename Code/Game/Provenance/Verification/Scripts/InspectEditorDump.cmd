@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
pushd "%~dp0..\Artifacts\Landscape"
cl /nologo /EHsc /std:c++17 /O2 /Fe:InspectEditorDump.exe /Fo:InspectEditorDump.obj "%~dp0..\InspectEditorDump.cpp"
if errorlevel 1 (popd & exit /b 2)
InspectEditorDump.exe %*
set "dumpResult=%errorlevel%"
popd
exit /b %dumpResult%
