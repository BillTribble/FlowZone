#include "StateBroadcaster.h"

namespace flowzone {

StateBroadcaster::StateBroadcaster(WebSocketServer &s) : server(s) {}
StateBroadcaster::~StateBroadcaster() {}

void StateBroadcaster::broadcast(const AppState &state) {
  // In a real impl, serialize state to JSON using nlohmann/json
  // For Phase 2 stub:
  // json j; j["bpm"] = state.bpm; ...
  // server.send(j.dump());
  juce::ignoreUnused(state);
}

void StateBroadcaster::sendError(ErrorCode code, const juce::String &message) {
  // server.sendError(...)
  juce::ignoreUnused(code, message);
}

} // namespace flowzone
