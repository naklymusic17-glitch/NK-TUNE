cmake_minimum_required(VERSION 3.22)
project(NKLTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Descarga JUCE automaticamente (necesita git e internet la primera vez).
# Si ya tenes JUCE en una carpeta "JUCE" al lado de este archivo, borra el bloque
# FetchContent y usa: add_subdirectory(JUCE)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# En GitHub Actions (variable CI) no se copia el plugin a carpetas del sistema
set(NKL_COPY TRUE)
if(DEFINED ENV{CI})
    set(NKL_COPY FALSE)
endif()

juce_add_binary_data(NKLAssets SOURCES Source/Assets/background.png)
set_target_properties(NKLAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

juce_add_plugin(NKLTune
    COMPANY_NAME "NKL"
    PLUGIN_MANUFACTURER_CODE Nklt
    PLUGIN_CODE Nkl1
    FORMATS VST3
    PRODUCT_NAME "NKL TUNE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD ${NKL_COPY})

target_sources(NKLTune PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(NKLTune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NKLTune
    PRIVATE
        NKLAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
