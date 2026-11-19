@echo off
setlocal
for /f "usebackq tokens=*" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -version "[18.0,19.0)" -requires Microsoft.Component.MSBuild -property installationPath`) do set "VS2026=%%I"
if not defined VS2026 (
  echo Visual Studio 2026 with MSBuild is required.
  exit /b 1
)
set "MSBUILD=%VS2026%\MSBuild\Current\Bin\amd64\MSBuild.exe"
"%MSBUILD%" "%~dp0..\Game.sln" -m -t:Build -p:Configuration=Retail -p:Platform=x64
if errorlevel 1 exit /b %errorlevel%
"%MSBUILD%" "%~dp0..\Game.sln" -m -t:Build "-p:Configuration=Retail Server" -p:Platform=x64
exit /b %errorlevel%