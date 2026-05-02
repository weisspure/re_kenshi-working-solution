@echo off
call "%~dp0build.bat"
if errorlevel 1 exit /b %ERRORLEVEL%
"%~dp0x64\Release\RaceChange_Tests.exe"
exit /b %ERRORLEVEL%

