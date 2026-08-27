@echo off
setlocal
pushd "%~dp0"
call PLAY_ESOTERICA_ADOPT_PAGE.cmd
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
