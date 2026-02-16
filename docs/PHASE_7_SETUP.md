# Phase 7: Plugin Isolation - Manual Setup Required

## Overview
Phase 7 implements isolated plugin hosting via separate processes with shared memory IPC. This requires manual Projucer configuration and Xcode builds.

## Implementation Status
- ✅ Architecture defined
- ✅ Code scaffolding created
- ⏸️ **BLOCKED:** Requires Projucer configuration and Xcode

## Required Manual Steps

### 1. Configure PluginHostApp in Projucer

1. Open `FlowZone.jucer` in Projucer
2. Add new Console Application target:
   - Target Name: `PluginHostApp`
   - Type: Console Application
   - Main File: `src/host/PluginHostMain.cpp`
3. Configure target settings:
   - Deployment Target: macOS 11.0+
   - Architecture: Apple Silicon Native
   - Link JUCE modules: `juce_audio_processors`, `juce_core`, `juce_events`
4. Add source files to target:
   - `src/host/PluginHostMain.cpp`
   - `src/host/PluginHostProcess.h`
   - `src/host/PluginHostProcess.cpp` (to be created)
5. Save and export to Xcode

### 2. Implement PluginHostProcess.cpp

Create `src/host/PluginHostProcess.cpp` with:
- Shared memory initialization using `juce::SharedResourcePointer`
- Ring buffer implementation (4× audio buffer size)
- Plugin loading via `juce::AudioPluginFormatManager`
- Audio processing loop
- Heartbeat mechanism (500ms interval)

### 3. Build in Xcode

```bash
cd Builds/MacOSX
xcodebuild -project FlowZone.xcodeproj -scheme PluginHostApp -configuration Debug
```

### 4. Test IPC Round-Trip

Test data from `tests/engine/PluginHost_IPC_Test.cpp`:
- Launch PluginHostApp with manufacturer ID
- Write audio buffer to shared memory
- Verify host processes buffer
- Read processed audio back
- Measure latency (should be <10ms)

## Architecture Details

### Shared Memory Layout
```
SharedAudioRingBuffer {
    Header {
        writePos: atomic<uint32>
        readPos: atomic<uint32>
        dropCount: atomic<uint32>
        active: atomic<bool>
        bufferSize: uint32
        sampleRate: uint32
    }
    audioData[channels × samples × 4]  // 4× multiplier for ring buffer
}
```

### Communication Flow
```
FlowZone Main Process
    ↓ (launch)
PluginHostApp --manufacturer_id-->
    ↓ (setup shared memory)
SharedAudioRingBuffer (mmap)
    ↓ (write audio)
PluginHostApp processes
    ↓ (write back)
FlowZone reads result
    ↓ (heartbeat every 500ms)
Watchdog monitors health
```

### Crash Isolation
- Each manufacturer runs in separate process
- VST crash only kills that manufacturer's host
- Watchdog detects missed heartbeats (3× = 1.5s timeout)
- Exponential backoff respawn: immediate → 2s → 10s → mark Suspended

## Files Created

- `src/host/PluginHostMain.cpp` - Entry point for host process
- `src/host/PluginHostProcess.h` - Core host process management
- `src/host/PluginHostProcess.cpp` - **TO BE CREATED**
- `tests/engine/PluginHost_IPC_Test.cpp` - **TO BE CREATED**

## Next Steps After Manual Setup

1. Complete `PluginHostProcess.cpp` implementation
2. Create IPC tests
3. Implement watchdog in main process (`src/engine/PluginProcessManager.cpp`)
4. Add hot-swap and plugin scanning
5. Test with real VST3 plugins

## Why Manual Setup is Required

- Projucer is a GUI application - no CLI API
- Xcode required for macOS binary builds
- Shared memory IPC needs real process testing
- Cannot be fully automated in this environment
