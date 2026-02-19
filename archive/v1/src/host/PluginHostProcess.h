/**
 * PluginHostProcess - Core process management for isolated plugin hosting
 * 
 * Manages:
 * - Shared memory ring buffer communication
 * - Plugin loading and audio processing
 * - Heartbeat protocol for crash detection
 * - Clean shutdown and resource cleanup
 */

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>

namespace flowzone {

// Shared memory ring buffer for audio I/O
// Ring buffer size: 4 × audio buffer size per host
struct SharedAudioRingBuffer {
    static constexpr int BUFFER_SIZE_MULTIPLIER = 4;
    static constexpr int MAX_CHANNELS = 2;
    static constexpr int MAX_SAMPLES = 2048; // Max expected block size
    
    struct Header {
        std::atomic<uint32_t> writePos{0};
        std::atomic<uint32_t> readPos{0};
        std::atomic<uint32_t> dropCount{0};
        std::atomic<bool> active{false};
        uint32_t bufferSize;
        uint32_t sampleRate;
    };
    
    Header header;
    float audioData[MAX_CHANNELS * MAX_SAMPLES * BUFFER_SIZE_MULTIPLIER];
};

class PluginHostProcess {
public:
    explicit PluginHostProcess(const juce::String &manufacturerId);
    ~PluginHostProcess();
    
    bool initialize();
    void run();
    void shutdown();
    
    // Heartbeat for watchdog
    void sendHeartbeat();
    
private:
    juce::String manufacturerId;
    std::unique_ptr<juce::InterprocessConnection> ipcConnection;
    std::shared_ptr<SharedAudioRingBuffer> sharedMemory;
    
    std::atomic<bool> shouldRun{true};
    std::atomic<uint64_t> lastHeartbeatMs{0};
    
    juce::OwnedArray<juce::AudioPluginInstance> loadedPlugins;
    
    bool setupSharedMemory();
    bool connectToMainProcess();
    void processAudioLoop();
    void heartbeatLoop();
    
    static constexpr int HEARTBEAT_INTERVAL_MS = 500;
};

} // namespace flowzone
