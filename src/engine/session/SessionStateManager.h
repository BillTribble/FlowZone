#pragma once
#include "../state/AppState.h"
#include <JuceHeader.h>
#include <atomic>

namespace flowzone {

class SessionStateManager : private juce::Timer {
public:
  SessionStateManager();
  ~SessionStateManager();

  // Session lifecycle
  juce::Result saveSession(const juce::File &file, const AppState &state);
  juce::Result loadSession(const juce::File &file, AppState &outState);
  juce::Result createNewSession(const juce::String &name,
                                const juce::File &parentDir,
                                AppState &outState);

  // Autosave
  void startAutosave(const juce::File &sessionDir);
  void stopAutosave();
  juce::Result performAutosave();
  
  // Crash recovery
  juce::Result loadLastAutosave(const juce::File &sessionDir, AppState &outState);
  bool hasAutosave(const juce::File &sessionDir) const;

  // Riff history management
  juce::Result commitRiff(const juce::String &name, int layers,
                         const std::vector<juce::String> &colors,
                         const juce::String &userId);
  juce::Result loadRiff(const juce::String &riffId);
  juce::Result deleteJam(const juce::String &sessionId);

  // State access
  AppState getCurrentState() const { return currentState; }
  void updateState(std::function<void(AppState &)> modifier);
  void setState(const AppState &state) { currentState = state; }

private:
  AppState currentState;
  juce::File autosaveDir;
  std::atomic<bool> autosaveEnabled{false};
  
  // Timer callback for autosave (every 30s)
  void timerCallback() override;
  
  // Logging
  void logSessionEvent(const juce::String &event, const juce::String &details = "");
  
  juce::CriticalSection stateLock;
};

} // namespace flowzone
