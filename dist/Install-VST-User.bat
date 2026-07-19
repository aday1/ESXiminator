@echo off
setlocal EnableExtensions
title ESXiminator VST3 Setup (per-user, no Admin)

rem Installs into the per-user VST3 folder — no elevation required.
rem Prefer Install-VST.bat (system) if your DAW only scans Common Files.

set "SRC=%~dp0ESXiminator.vst3"
set "DST=%LOCALAPPDATA%\Programs\Common\VST3\ESXiminator.vst3"

if not exist "%SRC%\Contents\x86_64-win\ESXiminator.vst3" (
  echo ERROR: ESXiminator.vst3 not found next to this script.
  pause
  exit /b 1
)

echo.
echo ESXiminator VST3 Setup (per-user)
echo ---------------------------------
echo Source: %SRC%
echo Dest:   %DST%
echo.

if exist "%DST%" rmdir /s /q "%DST%"
mkdir "%LOCALAPPDATA%\Programs\Common\VST3" 2>nul
xcopy /e /i /y "%SRC%" "%DST%" >nul
if errorlevel 1 (
  echo ERROR: copy failed.
  pause
  exit /b 1
)

echo Installed to per-user VST3 folder.
echo If your DAW does not see it, run Install-VST.bat instead (system install).
echo.
pause
exit /b 0
