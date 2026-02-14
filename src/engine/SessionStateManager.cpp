#include "SessionStateManager.h"

namespace flowzone {

SessionStateManager::SessionStateManager() {}

void SessionStateManager::updateBpm(double bpm) { currentState.bpm = bpm; }

void SessionStateManager::setPlaying(bool isPlaying) {
  currentState.isPlaying = isPlaying;
}

} // namespace flowzone
