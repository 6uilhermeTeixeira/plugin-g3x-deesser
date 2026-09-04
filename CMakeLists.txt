cmake_minimum_required(VERSION 3.25)

project(G3XDeEsser VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(G3XDeEsser
    COMPANY_NAME "G3X Audio"
    BUNDLE_ID "com.g3xaudio.deesser"
    PLUGIN_MANUFACTURER_CODE G3xA
    PLUGIN_CODE DsGx
    FORMATS VST3 Standalone
    PRODUCT_NAME "G3X DeEsser"
    DESCRIPTION "Musical split-band and wideband de-esser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
)

target_sources(G3XDeEsser PRIVATE
    Source/DeEsserEngine.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
    Source/dsp/DeEsser.cpp
)
target_include_directories(G3XDeEsser PRIVATE Source)
target_compile_definitions(G3XDeEsser PUBLIC
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
)
target_link_libraries(G3XDeEsser PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

enable_testing()
add_executable(G3XDeEsserDspTests
    tests/DeEsserTests.cpp
    Source/DeEsserEngine.cpp
    Source/dsp/DeEsser.cpp
)
target_include_directories(G3XDeEsserDspTests PRIVATE Source)
target_compile_features(G3XDeEsserDspTests PRIVATE cxx_std_20)
target_link_libraries(G3XDeEsserDspTests PRIVATE juce::juce_dsp)
target_compile_definitions(G3XDeEsserDspTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)
if(MSVC)
    target_compile_options(G3XDeEsserDspTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(G3XDeEsserDspTests PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
add_test(NAME G3XDeEsserDspTests COMMAND G3XDeEsserDspTests)
