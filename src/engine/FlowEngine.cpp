#include "FlowEngine.h"

namespace flowzone {

FlowEngine::FlowEngine() {}
FlowEngine::~FlowEngine() {}

void FlowEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  transport.prepareToPlay(sampleRate, samplesPerBlock);
}

void FlowEngine::processBlock(juce::AudioBuffer<float> &buffer,
                              juce::MidiBuffer &midiMessages) {
  // Process incoming commands from Message Thread
  processCommands();

  transport.processBlock(buffer, midiMessages);
  // TODO: Process Loopers
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

  broadcaster.broadcastFullState(state);
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
