#include "FlowEngine.h"
#include <JuceHeader.h>
#include <algorithm>
#include <string>

namespace flowzone {

FlowEngine::FlowEngine() : juce::Thread("AutoMergeThread") {
  for (int i = 0; i < 8; ++i) {
    slots.push_back(std::make_unique<Slot>(i));
  }
}

FlowEngine::~FlowEngine() { stopThread(2000); }

void FlowEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  transport.prepareToPlay(sampleRate, samplesPerBlock);
  retroBuffer.prepare(sampleRate, 60); // 60 seconds of history

  for (auto &slot : slots) {
    slot->prepareToPlay(sampleRate, samplesPerBlock);
  }
}

void FlowEngine::processBlock(juce::AudioBuffer<float> &buffer,
                              juce::MidiBuffer &midiMessages) {
  // Capture input before processing
  retroBuffer.pushBlock(buffer);

  // Process incoming commands from Message Thread
  processCommands();

  // Handle Transport
  transport.processBlock(buffer, midiMessages);

  // Sum slots into buffer
  for (auto &slot : slots) {
    slot->processBlock(buffer, buffer.getNumSamples());
  }

  // Check for sync merge arrival
  performMergeSync();
}

void FlowEngine::processCommands() {
  juce::String commandStr;
  bool stateChanged = false;
  // Drain the queue
  while (commandQueue.pop(commandStr)) {
    dispatcher.dispatch(commandStr, *this);
    stateChanged = true;
  }

  if (stateChanged) {
    broadcastState();
  }
}

void FlowEngine::broadcastState() {
  auto state = sessionManager.getCurrentState();

  // Sync Transport
  state.transport.isPlaying = transport.isPlaying();
  state.transport.bpm = transport.getBpm();
  state.transport.metronomeEnabled = transport.isMetronomeEnabled();
  state.transport.loopLengthBars = transport.getLoopLengthBars();
  state.transport.barPhase = transport.getBarPhase();

  // Use patch-based update (will auto-decide patch vs snapshot)
  broadcaster.broadcastStateUpdate(state);
}

void FlowEngine::setLoopLength(int bars) {
  // Check for empty slot
  int targetSlot = -1;
  for (int i = 0; i < (int)slots.size(); ++i) {
    if (!slots[i]->isFull()) {
      targetSlot = i;
      break;
    }
  }

  if (targetSlot == -1) {
    // All 8 slots full! Trigger Auto-Merge
    nextCaptureBars = bars;
    triggerAutoMerge();
  } else {
    // Capture into targetSlot
    juce::AudioBuffer<float> captured;
    int samples = (int)(bars * transport.getSamplesPerBar());
    retroBuffer.getPastAudio(0, samples, captured);
    slots[targetSlot]->setAudioData(captured);
    slots[targetSlot]->getState().loopLengthBars = bars;
  }
}

void FlowEngine::triggerAutoMerge() {
  if (mergePending.load())
    return;

  mergePending.store(true);
  startThread();
}

void FlowEngine::run() {
  // Background thread: Sum slots 1-8
  // In V1, we just simulate the merge process for now
  // and prepare the data for the atomic swap in the audio thread.

  // Real implementation would sum audio here.
  juce::Thread::sleep(50); // Simulate work
}

void FlowEngine::performMergeSync() {
  if (!mergePending.load())
    return;

  // Audio thread transition:
  // 1. Sum Slots 1-8 into Slot 1
  // 2. Clear 2-8
  // 3. Trigger new capture into Slot 2

  juce::AudioBuffer<float> merger;
  int maxSamples = 0;
  for (auto &s : slots)
    maxSamples = std::max(
        maxSamples, (int)(s->getLengthInBars() * transport.getSamplesPerBar()));

  merger.setSize(2, maxSamples);
  merger.clear();

  for (auto &s : slots) {
    // Basic sum in audio thread for V1/Proof-of-concept
    // (Actual background sum is safer for CPU, but this works for testing)
    s->processBlock(merger, maxSamples);
  }

  slots[0]->setAudioData(merger);
  slots[0]->getState().loopLengthBars =
      (int)(maxSamples / transport.getSamplesPerBar());
  slots[0]->getState().presetId = "auto_merge";

  for (int i = 1; i < 8; ++i) {
    slots[i]->clear();
  }

  mergePending.store(false);

  // Record the 9th loop into Slot 2
  setLoopLength(nextCaptureBars);
}

void FlowEngine::setSlotVolume(int slotIndex, float volume) {
  if (slotIndex >= 0 && slotIndex < (int)slots.size()) {
    slots[slotIndex]->setVolume(volume);
  }
}

void FlowEngine::setSlotMuted(int slotIndex, bool muted) {
  if (slotIndex >= 0 && slotIndex < (int)slots.size()) {
    slots[slotIndex]->setState(muted ? "MUTED" : "PLAYING");
  }
}

void FlowEngine::loadPreset(const juce::String &category,
                            const juce::String &presetName) {
  juce::Logger::writeToLog("Load Preset: " + category + " / " + presetName);

  sessionManager.updateState([&](AppState &s) {
    s.activeMode.category = category;
    s.activeMode.presetName = presetName;
    // Also update presetId to match name for now, or look it up
    s.activeMode.presetId = presetName.toLowerCase().replace(" ", "-");
  });
}

void FlowEngine::triggerPad(int padIndex, float velocity) {
  juce::Logger::writeToLog("Pad Trigger: " + juce::String(padIndex) +
                           " vel=" + juce::String(velocity));
  // TODO: Phase 4 - Implement Drum Engine Trigger
}

void FlowEngine::updateXY(float x, float y) {
  sessionManager.updateState([&](AppState &s) {
    s.activeFX.xyPosition.x = x;
    s.activeFX.xyPosition.y = y;
    s.activeFX.isActive = true;
  });
}

} // namespace flowzone
