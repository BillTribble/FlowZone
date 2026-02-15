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

  // Broadcast the full state
  void broadcastFullState(const AppState &state);

  // Get current revision ID
  int64_t getRevisionId() const;

private:
  MessageCallback sendMessage;
  int64_t revisionId = 0;

  // Thread safety
  juce::CriticalSection lock;
};

} // namespace flowzone
