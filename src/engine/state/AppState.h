#pragma once
#include <JuceHeader.h>
#include <array>
#include <string>
#include <vector>

namespace flowzone {

struct PluginInstance {
  juce::String id;
  juce::String pluginId;
  juce::String manufacturer;
  juce::String name;
  bool bypass = false;
  juce::String state; // Base64
};

struct SlotState {
  juce::String id;
  juce::String state = "EMPTY"; // EMPTY, PLAYING, MUTED
  float volume = 1.0f;
  bool muted = false;
  juce::String riffId;
  juce::String name;
  juce::String instrumentCategory = "drums";
  juce::String presetId = "mic_input";
  juce::String userId = "local";
  std::vector<PluginInstance> pluginChain;
  int loopLengthBars = 4;
  double originalBpm = 120.0;
  int lastError = 0;
};

struct RiffHistoryEntry {
  juce::String id;
  int64_t timestamp = 0;
  juce::String name;
  int layers = 0;
  std::vector<juce::String> colors;
  juce::String userId = "local";
};

struct AppState {
  struct Session {
    juce::String id;
    juce::String name;
    juce::String emoji;
    int64_t createdAt = 0;
  };

  std::vector<Session> sessions;
  Session session;

  struct Transport {
    double bpm = 120.0;
    bool isPlaying = false;
    double barPhase = 0.0;
    int loopLengthBars = 4;
    bool metronomeEnabled = false;
    bool quantiseEnabled = false;
    int rootNote = 0;
    juce::String scale = "chromatic";
  } transport;

  struct ActiveMode {
    juce::String category = "drums";
    juce::String presetId = "synthetic";
    juce::String presetName = "Synthetic";
    bool isFxMode = false;
    std::vector<int> selectedSourceSlots;
  } activeMode;

  struct ActiveFX {
    juce::String effectId;
    juce::String effectName;
    struct {
      float x = 0.5f;
      float y = 0.5f;
    } xyPosition;
    bool isActive = false;
  } activeFX;

  struct Mic {
    float inputGain =
        0.0f; // dB? Spec says -60 to +40, but likely represented as float here
    float inputLevel = 0.0f; // 0.0 to 1.0 peak level for metering
    bool monitorInput = false;
    bool monitorUntilLooped = false;
  } mic;

  struct Looper {
    float inputLevel =
        0.0f; // 0.0 to 1.0 peak level for retrospective buffer input
    std::vector<float>
        waveformData; // Downsampled waveform for UI visualization (256 samples)
  } looper;

  std::vector<SlotState> slots;
  std::vector<RiffHistoryEntry> riffHistory;

  struct Settings {
    juce::String riffSwapMode = "instant";
    int bufferSize = 512;
    double sampleRate = 44100.0;
    juce::String storageLocation;
  } settings;

  struct System {
    float cpuLoad = 0.0f;
    float diskBufferUsage = 0.0f;
    float memoryUsageMB = 0.0f;
    int activePluginHosts = 0;
  } system;

  // Convert to JUCE var (JSON-compatible object)
  juce::var toVar() const;

  // Create from JUCE var
  static AppState fromVar(const juce::var &v);
};

} // namespace flowzone
