#include "CommandDispatcher.h"
#include "CommandQueue.h"
#include "CrashGuard.h"
#include "DrumEngine.h"
#include "SynthEngine.h"
#include "MicProcessor.h"
#include "RetrospectiveBuffer.h"
#include "FeatureExtractor.h"
#include "Slot.h"
#include "session/SessionStateManager.h"
#include "state/StateBroadcaster.h"
#include "transport/TransportService.h"
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace flowzone {

// bd-3uw: FlowEngine Skeleton
// Coordinates DSP graph, loopers, and transport
class FlowEngine : public juce::Thread {
public:
  FlowEngine();
  ~FlowEngine();

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages);

  TransportService &getTransport() { return transport; }
  CommandDispatcher &getDispatcher() { return dispatcher; }
  StateBroadcaster &getBroadcaster() { return broadcaster; }
  SessionStateManager &getSessionManager() { return sessionManager; }
  CommandQueue &getCommandQueue() { return commandQueue; }

  // Command Handlers (called by Dispatcher)
  void loadPreset(const juce::String &category, const juce::String &presetName);
  void setActiveCategory(const juce::String &category);
  void loadRiff(const juce::String &riffId);
  void triggerPad(int padIndex, float velocity);
  void releasePad(int padIndex);
  void updateXY(float x, float y);
  void setLoopLength(int bars);
  void setSlotVolume(int slotIndex, float volume);
  void setSlotMuted(int slotIndex, bool muted);
  
  // Mic controls
  void setInputGain(float gainDb);
  void toggleMonitorInput();
  void toggleMonitorUntilLooped();
  
  // Panic - stop all notes
  void panic();
  
  // Session management
  void createNewJam();
  void loadJam(const juce::String &sessionId);
  void renameJam(const juce::String &sessionId, const juce::String &name, const juce::String &emoji);
  void deleteJam(const juce::String &sessionId);

  // Background Thread for Auto-Merge
  void run() override;

private:
  TransportService transport;
  CommandDispatcher dispatcher;
  StateBroadcaster broadcaster;
  SessionStateManager sessionManager;
  CrashGuard crashGuard;
  RetrospectiveBuffer retroBuffer;
  FeatureExtractor featureExtractor;
  CommandQueue commandQueue;

  // Audio engines
  engine::DrumEngine drumEngine;
  engine::SynthEngine synthEngine;
  engine::MicProcessor micProcessor;
  
  // Active MIDI buffer for triggered notes
  juce::MidiBuffer activeMidi;
  double currentSampleRate = 44100.0;

  std::vector<std::unique_ptr<Slot>> slots;

  // Merge logic
  juce::CriticalSection mergeLock;
  std::atomic<bool> mergePending{false};
  int nextCaptureBars = 0;

  void processCommands();
  void broadcastState();
  void performMergeSync();
  void triggerAutoMerge();
};

} // namespace flowzone
