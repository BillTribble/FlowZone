#pragma once
#include "../shared/protocol/schema.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-n3n: Session State Manager
// Manages the source of truth for AppState
class SessionStateManager {
public:
  SessionStateManager();

  const AppState &getState() const { return currentState; }

  void updateBpm(double bpm);
  void setPlaying(bool isPlaying);

  void save(const juce::File &file);
  void load(const juce::File &file);

private:
  AppState currentState;
};

} // namespace flowzone
