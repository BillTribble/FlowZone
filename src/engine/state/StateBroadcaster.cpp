#include "StateBroadcaster.h"

namespace flowzone {

StateBroadcaster::StateBroadcaster() {}

StateBroadcaster::~StateBroadcaster() {}

void StateBroadcaster::setMessageCallback(MessageCallback callback) {
  juce::ScopedLock sl(lock);
  sendMessage = callback;
}

void StateBroadcaster::broadcastFullState(const AppState &state) {
  juce::ScopedLock sl(lock);
  revisionId++;

  juce::DynamicObject *root = new juce::DynamicObject();
  root->setProperty("cmd", "STATE_FULL");
  root->setProperty("revisionId", revisionId);
  root->setProperty("state", state.toVar());

  if (sendMessage) {
    juce::String jsonString = juce::JSON::toString(juce::var(root));
    sendMessage(jsonString);
  }
}

int64_t StateBroadcaster::getRevisionId() const { return revisionId; }

} // namespace flowzone
