#pragma once
#include "../state/AppState.h"
#include <JuceHeader.h>

namespace flowzone {

class SessionStateManager {
public:
  SessionStateManager();
  ~SessionStateManager();

  juce::Result saveSession(const juce::File &file, const AppState &state);
  juce::Result loadSession(const juce::File &file, AppState &outState);

  // Create a new session structure on disk if needed
  juce::Result createNewSession(const juce::String &name,
                                const juce::File &parentDir,
                                AppState &outState);

private:
};

} // namespace flowzone
