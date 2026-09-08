@echo off
setlocal
if not "%~1"=="baseline" if not "%~1"=="after" exit /b 2
for %%I in ("%~dp0..\..\..\..\..") do set "SPLIT_ROOT=%%~fI"
rem Baseline mode is only valid before the extraction exists; never relabel new output as old evidence.
if "%~1"=="baseline" if exist "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteSpallResolution.cpp" exit /b 2
set "SPLIT_OUTPUT=%SPLIT_ROOT%\Build\Verification\GraniteGeometrySplit\%~1"
if not exist "%SPLIT_OUTPUT%" mkdir "%SPLIT_OUTPUT%"
if errorlevel 1 exit /b 1
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
set "SPLIT_SOURCE="
set "SPLIT_OBJECT="
if exist "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteSpallResolution.cpp" (
    set SPLIT_SOURCE="%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteSpallResolution.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteFormation.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteRootedDetachment.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteRootedPartition.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteFracture.cpp"
    set SPLIT_OBJECT="%SPLIT_OUTPUT%\GraniteSpallResolution.obj" "%SPLIT_OUTPUT%\GraniteFormation.obj" "%SPLIT_OUTPUT%\GraniteRootedDetachment.obj" "%SPLIT_OUTPUT%\GraniteRootedPartition.obj" "%SPLIT_OUTPUT%\GraniteFracture.obj"
)
cl /c /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\APPLICATIONS\PROVENANCEVERIFIER\\" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\EXTERNAL\OPTICK\INCLUDE" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\BASE\THIRDPARTY\IMGUI\\" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\BASE\THIRDPARTY\EA\\" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\BASE\THIRDPARTY\EA\EASTL\INCLUDE\\" /I"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\BASE\THIRDPARTY\EA\EABASE\INCLUDE\COMMON\\" /Zi /JMC /nologo /W3 /WX /diagnostics:column /Od /Ob1 /Oi /D EE_PROVENANCE_STANDALONE_AUTHORITY=1 /D EE_DEBUG=1 /D EE_DLL /D NDEBUG /D NOMINMAX /D WIN32_LEAN_AND_MEAN /D _CRT_SECURE_NO_WARNINGS /D ESOTERICA_APPLICATIONS_PROVENANCEVERIFIER /D _HAS_EXCEPTIONS=0 /D EASTL_DLL /D "EASTL_USER_CONFIG_HEADER=<eastl_Esoterica.h>" /Gm- /RTC1 /MD /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /std:c++20 /Fo"%SPLIT_OUTPUT%\\" /Fd"%SPLIT_OUTPUT%\vc145.pdb" /external:W3 /Gd /TP /wd4865 /wd4189 /wd4946 /wd4191 /wd5220 /wd4255 /wd4266 /wd4623 /wd4711 /wd4388 /wd4355 /wd5031 /wd4774 /wd4582 /wd4365 /wd5246 /wd4061 /wd5219 /wd4514 /wd4505 /wd4251 /wd4100 /wd4127 /wd4201 /wd4577 /wd4464 /wd4668 /wd4710 /wd4820 /wd5052 /wd4619 /wd4625 /wd4626 /wd5026 /wd5027 /wd5045 /wd26495 /wd5262 /wd5264 /wd5267 /FC /bigobj /Zo /Zc:inline "%SPLIT_ROOT%\Code\Applications\ProvenanceVerifier\Main.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\MaterialGeometry.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Geometry\GraniteGeometry.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Systems\ProvenanceSurfaceEvaluation.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Verification\GraniteAuthorityVerifier.cpp" "%SPLIT_ROOT%\Code\Game\Provenance\Verification\SurfaceAuthorityVerifier.cpp" %SPLIT_SOURCE%
if errorlevel 1 exit /b %errorlevel%
link /OUT:"%SPLIT_OUTPUT%\ESOTERICA.APPLICATIONS.PROVENANCEVERIFIER.EXE" /INCREMENTAL /ILK:"%SPLIT_OUTPUT%\ESOTERICA.APPLICATIONS.PROVENANCEVERIFIER.ILK" /NOLOGO /LIBPATH:"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\EXTERNAL\OPTICK\LIB\X64\RELEASE\\" /FUNCTIONPADMIN OPTICKCORE.LIB DXCOMPILER.LIB DXGUID.LIB D3D12.LIB DXGI.LIB XINPUT.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /manifestinput:"C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\CODE\ESOTERICA.MANIFEST" /DEBUG:FULL /PDB:"%SPLIT_OUTPUT%\ESOTERICA.APPLICATIONS.PROVENANCEVERIFIER.PDB" /SUBSYSTEM:CONSOLE /OPT:NOREF /OPT:NOICF /LTCGOUT:"%SPLIT_OUTPUT%\ESOTERICA.APPLICATIONS.PROVENANCEVERIFIER.IOBJ" /TLBID:1 /DYNAMICBASE:NO /NXCOMPAT /IMPLIB:"%SPLIT_OUTPUT%\ESOTERICA.APPLICATIONS.PROVENANCEVERIFIER.LIB" /MACHINE:X64 "%SPLIT_OUTPUT%\MAIN.OBJ" "%SPLIT_OUTPUT%\MATERIALGEOMETRY.OBJ" "%SPLIT_OUTPUT%\GRANITEGEOMETRY.OBJ" "%SPLIT_OUTPUT%\PROVENANCESURFACEEVALUATION.OBJ" "%SPLIT_OUTPUT%\GRANITEAUTHORITYVERIFIER.OBJ" "%SPLIT_OUTPUT%\SURFACEAUTHORITYVERIFIER.OBJ" "C:\USERS\D-DAY\PROVENANCEWORKSPACE\CLIENT\PROVENANCECLIENT\BUILD\X64_DEBUG\ESOTERICA.BASE.LIB" %SPLIT_OBJECT%
if errorlevel 1 exit /b %errorlevel%
set "PATH=%SPLIT_ROOT%\Build\x64_Debug;%PATH%"
"%SPLIT_OUTPUT%\Esoterica.Applications.ProvenanceVerifier.exe" > "%SPLIT_OUTPUT%\report.txt"
set "SPLIT_EXIT=%errorlevel%"
type "%SPLIT_OUTPUT%\report.txt"
if not "%SPLIT_EXIT%"=="0" exit /b %SPLIT_EXIT%
if "%~1"=="after" (
    fc /L "%~dp0..\Fixtures\GraniteGeometrySplitBaseline.txt" "%SPLIT_OUTPUT%\report.txt"
    if errorlevel 1 exit /b 1
)
exit /b %SPLIT_EXIT%
