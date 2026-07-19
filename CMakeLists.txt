cmake_minimum_required(VERSION 3.22)
project(ESXiminator VERSION 2.0.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE location: pass -DJUCE_DIR=/path/to/JUCE or place JUCE next to this file
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
endif()
add_subdirectory(${JUCE_DIR} JUCE_build)

juce_add_plugin(ESXiminator
    COMPANY_NAME "Aday"
    PLUGIN_MANUFACTURER_CODE "AaEx"
    PLUGIN_CODE "Esx1"
    FORMATS VST3 Standalone
    PRODUCT_NAME "ESXiminator"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Tools"
)

target_sources(ESXiminator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UpdateChecker.cpp
)

if(MSVC)
    # without this MSVC reads the UTF-8 sources as ANSI and mangles non-ASCII labels
    target_compile_options(ESXiminator PUBLIC /utf-8)
endif()

target_compile_definitions(ESXiminator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ESXiminator PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    PUBLIC
    juce::juce_recommended_config_flags
)

if(WIN32)
    target_link_libraries(ESXiminator PRIVATE
        winmm ole32 ws2_32 version wininet shlwapi dwmapi uuid oleaut32 imm32 comdlg32 rpcrt4 gdi32 user32 kernel32 advapi32 shell32)
endif()
