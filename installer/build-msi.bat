@echo off
setlocal EnableExtensions
cd /d "%~dp0.."

set "VER="
if exist "dist\version.txt" for /f "usebackq delims=" %%V in ("dist\version.txt") do set "VER=%%V"
if "%VER%"=="" set "VER=2.0.3"

if not exist "dist\ESXiminator.exe" (
  echo ERROR: dist\ESXiminator.exe missing. Unzip a release into dist\ first.
  exit /b 1
)
if not exist "dist\ESXiminator.vst3\Contents\x86_64-win\ESXiminator.vst3" (
  echo ERROR: dist\ESXiminator.vst3 bundle missing.
  exit /b 1
)
if not exist "dist\ESXiminator-Hub.exe" (
  echo ERROR: dist\ESXiminator-Hub.exe missing.
  exit /b 1
)

if not exist "dist\GETTING_STARTED.md" copy /y "GETTING_STARTED.md" "dist\GETTING_STARTED.md" >nul
if not exist "dist\README.md" copy /y "README.md" "dist\README.md" >nul
if not exist "dist\version.txt" >"dist\version.txt" echo %VER%

set "OUT=dist\ESXiminator-v%VER%-Setup.msi"
echo Building %OUT% ...

wix build installer\Package.wxs installer\WixUI_ESXiminator.wxs ^
  -ext WixToolset.UI.wixext ^
  -arch x64 ^
  -d ProductVersion=%VER% ^
  -loc installer\en-us.wxl ^
  -b app=dist ^
  -b vst=dist\ESXiminator.vst3 ^
  -o "%OUT%"

if errorlevel 1 exit /b 1
dir "%OUT%"
echo OK
