# Publishing ESXiminator to GitHub

Repo: https://github.com/aday1/ESXiminator
Showcase: https://aday1.github.io/ESXiminator/
Release: https://github.com/aday1/ESXiminator/releases/latest

## Release assets
- ESXiminator-v*-Setup.msi — MSI (Hub + standalone + VST3; asks for VST folder, preferred default Common Files\VST3; payload next to Hub)
- ESXiminator-v*-win64.zip — portable
- ESXiminator-v*-VST3-Setup.zip — Hub + VST3

## Build MSI
WiX Toolset v7 on PATH, then:
  installer\build-msi.bat
