cmake_minimum_required(VERSION 3.22)
project(FlowZone VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE lives in libs/juce-binary/JUCE
add_subdirectory(libs/juce-binary/JUCE)

juce_add_gui_app(FlowZone
    PRODUCT_NAME "FlowZone"
    COMPANY_NAME "FlowZone"
    BUNDLE_ID "com.flowzone.prototype"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FlowZone needs microphone access for audio input processing."
)

target_sources(FlowZone PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/LevelMeter.cpp
    src/RetrospectiveBuffer.cpp
    src/WaveformPanel.cpp
    src/Riff.h
    src/RiffHistoryPanel.cpp
    src/RiffPlaybackEngine.cpp
)

target_compile_definitions(FlowZone PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:FlowZone,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:FlowZone,JUCE_VERSION>"
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(FlowZone PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
