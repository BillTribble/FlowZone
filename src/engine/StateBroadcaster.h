#pragma once
#include "../../shared/protocol/schema.h"
#include "server/WebSocketServer.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-34z: State Broadcaster
class StateBroadcaster {
public:
  StateBroadcaster(WebSocketServer &server);
  ~StateBroadcaster();

  void broadcast(const AppState &state);
  void sendError(ErrorCode code, const juce::String &message);

private:
  WebSocketServer &server;
};

} // namespace flowzone
