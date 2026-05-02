@echo off
set MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe
"%MSBUILD%" "%~dp0DialogueIdentityProbe.vcxproj" /p:Configuration=Release /p:Platform=x64
if errorlevel 1 exit /b %ERRORLEVEL%
copy /Y "%~dp0x64\Release\DialogueIdentityProbe.dll" "%~dp0DialogueIdentityProbe\DialogueIdentityProbe.dll"
exit /b %ERRORLEVEL%
