#pragma once
#include "../shared/protocol/schema.h"
#include <JuceHeader.h>

namespace flowzone {

class FlowEngine; // Forward declaration

// bd-14c: Command Dispatcher
class CommandDispatcher {
public:
  CommandDispatcher();
  ~CommandDispatcher();

  // Parse JSON command and dispatch to Engine
  void dispatch(const juce::String &jsonCommand, FlowEngine &engine);

private:
  void handlePlay(FlowEngine &engine);
  void handlePause(FlowEngine &engine);
  void handleTogglePlay(FlowEngine &engine);
  void handleToggleMetronome(FlowEngine &engine);
  void handleSetBpm(FlowEngine &engine, double bpm);
  void handleSetPreset(FlowEngine &engine, const juce::String &category,
                       const juce::String &preset);
  void handleSetMode(FlowEngine &engine, const juce::String &category);
  void handleLoadRiff(FlowEngine &engine, const juce::String &riffId);
  void handleNoteOn(FlowEngine &engine, int pad, float velocity);
  void handleNoteOff(FlowEngine &engine, int pad);
  void handleXYChange(FlowEngine &engine, float x, float y);
  void handleSetLoopLength(FlowEngine &engine, int bars);
  void handleSetSlotMuted(FlowEngine &engine, int index, bool muted);
  void handleSetSlotVolume(FlowEngine &engine, int index, float volume);
  void handleCommit(FlowEngine &engine);
  void handleSetInputGain(FlowEngine &engine, float gainDb);
  void handleToggleMonitorInput(FlowEngine &engine);
  void handleToggleMonitorUntilLooped(FlowEngine &engine);
  void handlePanic(FlowEngine &engine);
  void handleNewJam(FlowEngine &engine);
  void handleLoadJam(FlowEngine &engine, const juce::String &sessionId);
  void handleRenameJam(FlowEngine &engine, const juce::String &sessionId,
                       const juce::String &name, const juce::String &emoji);
  void handleDeleteJam(FlowEngine &engine, const juce::String &sessionId);
};

} // namespace flowzone
