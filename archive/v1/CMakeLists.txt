cmake_minimum_required(VERSION 3.15)
project(FlowZoneTests VERSION 0.1.0 LANGUAGES C CXX)

# Add JUCE
add_subdirectory(libs/JUCE)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Define TransportService Library (Now Engine Library)
add_library(flowzone_engine STATIC
    src/engine/transport/TransportService.cpp
    src/engine/transport/TransportService.h
    src/engine/state/AppState.cpp
    src/engine/state/AppState.h
    src/engine/state/StateBroadcaster.cpp
    src/engine/state/StateBroadcaster.h
    src/engine/DiskWriter.cpp
    src/engine/RetrospectiveBuffer.cpp
    src/engine/session/SessionStateManager.cpp
    src/engine/session/SessionStateManager.h
    src/engine/FlowEngine.cpp
    src/engine/FlowEngine.h
    src/engine/CommandDispatcher.cpp
    src/engine/CommandDispatcher.h
)

target_link_libraries(flowzone_engine PUBLIC
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
)

target_include_directories(flowzone_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode")

target_compile_features(flowzone_engine PUBLIC cxx_std_20)

# Define Test Executable
add_executable(engine_tests
    tests/engine/TransportService_Test.cpp
    tests/engine/StateBroadcaster_Test.cpp
    tests/engine/RetrospectiveBuffer_Test.cpp
    tests/engine/SessionStateManager_Test.cpp
)

target_link_libraries(engine_tests PRIVATE
    flowzone_engine
    Catch2::Catch2WithMain
)

target_compile_features(engine_tests PUBLIC cxx_std_20)

# --- CivetWeb Support ---
# Check if we have the sources, otherwise fetch or warn
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/civetweb/src/src/civetweb.c")
    add_library(civetweb_lib STATIC
        libs/civetweb/src/src/civetweb.c
    )
    target_include_directories(civetweb_lib PUBLIC
        libs/civetweb/include
    )
    target_compile_definitions(civetweb_lib PUBLIC USE_WEBSOCKET)
    # macOS defines
    if(APPLE)
        target_compile_definitions(civetweb_lib PUBLIC NO_SSL) 
    endif()
else()
    message(WARNING "CivetWeb sources not found in libs/civetweb/src. Networking will fail.")
endif()

# --- FlowZone Server Library ---
add_library(flowzone_server STATIC
    src/engine/server/WebSocketServer.cpp
    src/engine/server/WebSocketServer.h
)
target_link_libraries(flowzone_server PUBLIC
    civetweb_lib
    juce::juce_core
)
target_include_directories(flowzone_server PUBLIC
    libs/civetweb/include
    "${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode"
)
target_compile_features(flowzone_server PUBLIC cxx_std_20)

# --- FlowZone Standalone Executable ---
add_executable(FlowZone_Standalone
    src/Main.cpp
)

target_link_libraries(FlowZone_Standalone PRIVATE
    flowzone_engine
    flowzone_server
    civetweb_lib
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
)

target_include_directories(FlowZone_Standalone PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode")
target_compile_features(FlowZone_Standalone PUBLIC cxx_std_20)
