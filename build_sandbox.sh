#!/bin/bash
# Resumable cross-build of ESXiminator.vst3 (Windows x64) using zig c++.
# Safe to re-run: finished objects are kept, incomplete ones discarded.
SRC="$(cd "$(dirname "$0")" && pwd)"
J=/tmp/JUCE/modules
B=/tmp/pb
mkdir -p "$B/obj" "$B/inc"

# ---- plugin defines header (replaces juceaide output) ----
if [ ! -f "$B/inc/JucePluginDefines.h" ]; then
cat > "$B/inc/JucePluginDefines.h" <<'EOF'
#pragma once
#define JucePlugin_Build_VST3             1
#define JucePlugin_Name                   "ESXiminator"
#define JucePlugin_Desc                   "Korg Electribe ESX-1 remote control"
#define JucePlugin_Manufacturer           "Aday"
#define JucePlugin_ManufacturerWebsite    ""
#define JucePlugin_ManufacturerEmail      ""
#define JucePlugin_ManufacturerCode       0x41614578   // 'AaEx'
#define JucePlugin_PluginCode             0x45737831   // 'Esx1'
#define JucePlugin_IsSynth                0
#define JucePlugin_WantsMidiInput         1
#define JucePlugin_ProducesMidiOutput     1
#define JucePlugin_IsMidiEffect           1
#define JucePlugin_EditorRequiresKeyboardFocus 0
#define JucePlugin_Version                2.0.2
#define JucePlugin_VersionCode            0x20002
#define JucePlugin_VersionString          "2.0.2"
#define JucePlugin_VSTUniqueID            JucePlugin_PluginCode
#define JucePlugin_VSTCategory            kPlugCategEffect
#define JucePlugin_Vst3Category           "Fx|Tools"
#define JucePlugin_AAXIdentifier          com.antialias.esximinator
#define JucePlugin_AAXManufacturerCode    JucePlugin_ManufacturerCode
#define JucePlugin_AAXProductId           JucePlugin_PluginCode
#define JucePlugin_AAXCategory            0
#define JucePlugin_AAXDisableBypass       0
#define JucePlugin_AAXDisableMultiMono    0
#define JucePlugin_CFBundleIdentifier     com.antialias.esximinator
EOF
fi

MODDEFS="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 \
 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_events=1 \
 -DJUCE_MODULE_AVAILABLE_juce_data_structures=1 -DJUCE_MODULE_AVAILABLE_juce_graphics=1 \
 -DJUCE_MODULE_AVAILABLE_juce_gui_basics=1 -DJUCE_MODULE_AVAILABLE_juce_gui_extra=1 \
 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_audio_devices=1 \
 -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1 -DJUCE_MODULE_AVAILABLE_juce_audio_plugin_client=1 \
 -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1 -DJUCE_MODULE_AVAILABLE_juce_audio_utils=1 \
 -DJUCE_STANDALONE_APPLICATION=0 -DJUCE_WEB_BROWSER=0 -DJUCE_USE_CURL=0 \
 -DJUCE_DISPLAY_SPLASH_SCREEN=0 -DJUCE_USE_DIRECTWRITE=0 -DJUCE_DIRECT2D=0 \
 -DJUCE_WASAPI=1 -DJUCE_DIRECTSOUND=0 -DJUCE_ASIO=0 -DJUCE_USE_WINRT_MIDI=0 \
 -DJUCE_USE_MP3AUDIOFORMAT=0 -DJUCE_USE_LAME_AUDIO_FORMAT=0 -DJUCE_USE_WINDOWS_MEDIA_FORMAT=0 \
 -DJUCE_PLUGINHOST_VST3=0 -DJUCE_PLUGINHOST_LADSPA=0 \
 -DJUCE_VST3_CAN_REPLACE_VST2=0 -DJUCE_LOAD_CURL_SYMBOLS_LAZILY=0 \
 -DJUCE_MODAL_LOOPS_PERMITTED=0 -DUNICODE -D_UNICODE"

INCS="-I$B/inc -I$J -I$J/juce_audio_processors/format_types/VST3_SDK -I$SRC/Source"
CXX="python3 -m ziglang c++ -target x86_64-windows-gnu"
FLAGS="-std=c++17 -w -municode"

compile() { # name source optlevel
    local obj="$B/obj/$1.o"
    [ -f "$obj" ] && return 0
    if [ $SECONDS -gt 22 ]; then echo "TIMEBOX — rerun to continue"; exit 3; fi
    echo "compiling $1 ..."
    $CXX $FLAGS $3 $MODDEFS $INCS -c "$2" -o "$obj.tmp" && mv "$obj.tmp" "$obj" || exit 1
}

for m in core events data_structures audio_basics audio_devices; do
    compile "juce_$m" "$J/juce_$m/juce_$m.cpp" -O0
done
for m in graphics gui_basics gui_extra audio_processors audio_formats audio_utils; do
    compile "juce_$m" "$J/juce_$m/juce_$m.cpp" -O0
done
FI="-include $B/inc/JucePluginDefines.h"
compile vst3_client "$J/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp" "-O0 $FI"
compile processor "$SRC/Source/PluginProcessor.cpp" "-O0 $FI"
compile editor "$SRC/Source/PluginEditor.cpp" "-O0 $FI"
compile update_checker "$SRC/Source/UpdateChecker.cpp" "-O0 $FI"
compile standalone_main "$SRC/Source/MainStandalone.cpp" "-O0 $FI"

LIBS="-lwinmm -lole32 -lws2_32 -lversion -lwininet -lshlwapi -ldwmapi -luuid -loleaut32 -limm32 -lcomdlg32 -lrpcrt4 -lgdi32 -luser32 -lshell32 -ladvapi32 -ldxgi -static-libgcc"
MODOBJS=$(ls "$B"/obj/*.o | grep -v standalone_main)
if [ ! -f "$B/ESXiminator.vst3" ]; then
    if [ $SECONDS -gt 8 ]; then echo "TIMEBOX before vst3 link - rerun"; exit 3; fi
    echo "linking vst3 ..."
    $CXX $FLAGS -shared -s -o "$B/ESXiminator.vst3" $MODOBJS $LIBS || exit 1
fi
EXEOBJS=$(ls "$B"/obj/*.o | grep -v vst3_client)
if [ ! -f "$B/ESXiminator.exe" ]; then
    if [ $SECONDS -gt 8 ]; then echo "TIMEBOX before exe link - rerun"; exit 3; fi
    echo "linking exe ..."
    $CXX -std=c++17 -w -s -Wl,--subsystem,windows -o "$B/ESXiminator.exe" $EXEOBJS $LIBS || exit 1
fi
echo DONE
ls -la "$B/ESXiminator.vst3" "$B/ESXiminator.exe"
