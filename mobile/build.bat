@echo off
setlocal
cd /d "%~dp0"
call "%~dp0..\build_mobile.bat"
exit /b %ERRORLEVEL%
