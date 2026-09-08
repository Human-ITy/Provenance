@echo off
setlocal
rem Separate headless outputs; never links or replaces the live Game DLL.
set "MOVEMENT_OUTPUT=%~dp0..\..\..\..\..\Build\Verification\MovementBaseline"
if not exist "%MOVEMENT_OUTPUT%" mkdir "%MOVEMENT_OUTPUT%"
pushd "%MOVEMENT_OUTPUT%"
if errorlevel 1 exit /b 2
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (popd & exit /b 2)
cl /nologo /EHsc /std:c++17 /W4 /O2 /Fe:MovementDiagnostic.exe /Fo:MovementDiagnostic.obj "%~dp0..\GraniteMovementBaselineDiagnostic.cpp"
if errorlevel 1 (popd & exit /b 2)
MovementDiagnostic.exe > optimized.txt
set "OPT_RESULT=%errorlevel%"
type optimized.txt
cl /nologo /EHsc /std:c++17 /W4 /Od /Fe:MovementDiagnosticDebug.exe /Fo:MovementDiagnosticDebug.obj "%~dp0..\GraniteMovementBaselineDiagnostic.cpp"
if errorlevel 1 (popd & exit /b 2)
MovementDiagnosticDebug.exe > debug.txt
set "DBG_RESULT=%errorlevel%"
type debug.txt
popd
if not "%OPT_RESULT%"=="0" exit /b 1
if not "%DBG_RESULT%"=="0" exit /b 1
exit /b 0
