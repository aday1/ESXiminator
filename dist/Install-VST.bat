@echo off
setlocal EnableExtensions
title ESXiminator VST3 Setup

rem Installs ESXiminator.vst3 into the system VST3 folder.
rem Double-click this from the unzipped release folder (needs Admin once).

set "SRC=%~dp0ESXiminator.vst3"
set "DST=%CommonProgramFiles%\VST3\ESXiminator.vst3"

if not exist "%SRC%\Contents\x86_64-win\ESXiminator.vst3" (
  echo ERROR: ESXiminator.vst3 not found next to this script.
  echo Unzip the full release package and run Install-VST.bat from that folder.
  pause
  exit /b 1
)

net session >nul 2>&1
if errorlevel 1 (
  echo Requesting Administrator rights to install into:
  echo   %DST%
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b 0
)

echo.
echo ESXiminator VST3 Setup
echo ----------------------
echo Source: %SRC%
echo Dest:   %DST%
echo.

if exist "%DST%" (
  echo Removing previous install...
  rmdir /s /q "%DST%"
)

mkdir "%CommonProgramFiles%\VST3" 2>nul
xcopy /e /i /y "%SRC%" "%DST%" >nul
if errorlevel 1 (
  echo ERROR: copy failed.
  pause
  exit /b 1
)

echo Installed.
echo.
echo Next: rescan plugins in your DAW, then load ESXiminator on a MIDI track.
echo See GETTING_STARTED.md in this folder for MIDI setup.
echo.
pause
exit /b 0
