#include "CommandDispatcher.h"

namespace flowzone {

CommandDispatcher::CommandDispatcher() {}
CommandDispatcher::~CommandDispatcher() {}

void CommandDispatcher::dispatch(const juce::String &jsonCommand) {
  // In a real impl, parse JSON here.
  // Stub implementation:
  if (jsonCommand.contains("Play")) {
    handlePlay();
  } else if (jsonCommand.contains("Pause")) {
    handlePause();
  }
}

void CommandDispatcher::handlePlay() {
  // Trigger Transport Play
}

void CommandDispatcher::handlePause() {
  // Trigger Transport Pause
}

void CommandDispatcher::handleSetBpm(double bpm) { juce::ignoreUnused(bpm); }

} // namespace flowzone
