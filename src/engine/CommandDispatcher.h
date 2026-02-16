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
  void handleXYChange(FlowEngine &engine, float x, float y);
  void handleSetLoopLength(FlowEngine &engine, int bars);
  void handleSetSlotMuted(FlowEngine &engine, int index, bool muted);
  void handleSetSlotVolume(FlowEngine &engine, int index, float volume);
};

} // namespace flowzone
