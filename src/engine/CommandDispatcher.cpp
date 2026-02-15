#include "CommandDispatcher.h"

namespace flowzone {

CommandDispatcher::CommandDispatcher() {}
CommandDispatcher::~CommandDispatcher() {}

void CommandDispatcher::dispatch(const juce::String &jsonCommand) {
  auto jsonVar = juce::JSON::parse(jsonCommand);
  if (!jsonVar.isObject())
    return;

  auto cmdType = jsonVar["cmd"].toString();

  if (cmdType == "PLAY") {
    handlePlay();
  } else if (cmdType == "PAUSE") {
    handlePause();
  } else if (cmdType == "SET_TEMPO") {
    if (jsonVar.hasProperty("bpm")) {
      handleSetBpm((double)jsonVar["bpm"]);
    }
  }
  // Add other commands here
}

void CommandDispatcher::handlePlay() {
  // Trigger Transport Play
}

void CommandDispatcher::handlePause() {
  // Trigger Transport Pause
}

void CommandDispatcher::handleSetBpm(double bpm) { juce::ignoreUnused(bpm); }

} // namespace flowzone
