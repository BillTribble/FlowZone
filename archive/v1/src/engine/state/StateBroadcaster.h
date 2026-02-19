#pragma once
#include "AppState.h"
#include <JuceHeader.h>
#include <functional>

namespace flowzone {

class StateBroadcaster {
public:
  using MessageCallback = std::function<void(const juce::String &)>;

  StateBroadcaster();
  ~StateBroadcaster();

  // Setup the callback for sending messages to clients
  void setMessageCallback(MessageCallback callback);

  // Broadcast the full state (snapshot)
  void broadcastFullState(const AppState &state);

  // Broadcast state update (uses patches when efficient, snapshot otherwise)
  void broadcastStateUpdate(const AppState &state);

  // Get current revision ID
  int64_t getRevisionId() const;

private:
  MessageCallback sendMessage;
  int64_t revisionId = 0;
  juce::var previousStateVar; // Store previous state for diff generation
  bool hasPreviousState = false;

  // Thread safety
  juce::CriticalSection lock;

  // Generate RFC 6902 JSON Patch operations
  juce::Array<juce::var> generateJsonPatch(const juce::var &oldVal,
                                           const juce::var &newVal,
                                           const juce::String &path);

  // Calculate size of patch operations in bytes (UTF-8)
  size_t calculatePatchSize(const juce::Array<juce::var> &ops) const;
};

} // namespace flowzone
