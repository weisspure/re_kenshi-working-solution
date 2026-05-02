@echo off
set MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe
"%MSBUILD%" "%~dp0StatModification_Tests.vcxproj" /p:Configuration=Release /p:Platform=x64
exit /b %ERRORLEVEL%
