#pragma once
#include "../shared/protocol/schema.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-14c: Command Dispatcher
class CommandDispatcher {
public:
  CommandDispatcher();
  ~CommandDispatcher();

  // Parse JSON command and return CommandType + Payload (concept)
  // For Phase 2, we just stub the dispatch logic
  void dispatch(const juce::String &jsonCommand);

private:
  void handlePlay();
  void handlePause();
  void handleSetBpm(double bpm);
};

} // namespace flowzone
