#include "SessionStateManager.h"
#include "DiskWriter.h"

namespace flowzone {

SessionStateManager::SessionStateManager() {}

void SessionStateManager::updateBpm(double bpm) { currentState.bpm = bpm; }

void SessionStateManager::setPlaying(bool isPlaying) {
  currentState.isPlaying = isPlaying;
}

void SessionStateManager::save(const juce::File &file) {
  DiskWriter::saveState(currentState, file);
}

void SessionStateManager::load(const juce::File &file) {
  currentState = DiskWriter::loadState(file);
}

} // namespace flowzone
