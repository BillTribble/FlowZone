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
  // Drain the queue
  while (commandQueue.pop(commandStr)) {
    dispatcher.dispatch(commandStr, *this);
  }
}

void FlowEngine::loadPreset(const juce::String &category,
                            const juce::String &presetName) {
  juce::Logger::writeToLog("Load Preset: " + category + " / " + presetName);
  // TODO: Phase 4 - Implement Preset Loading
}

void FlowEngine::triggerPad(int padIndex, float velocity) {
  juce::Logger::writeToLog("Pad Trigger: " + juce::String(padIndex) +
                           " vel=" + juce::String(velocity));
  // TODO: Phase 4 - Implement Drum Engine Trigger
}

void FlowEngine::updateXY(float x, float y) {
  // juce::Logger::writeToLog("XY: " + juce::String(x) + ", " +
  // juce::String(y));
  // TODO: Phase 4 - Update FX Parameters
}

} // namespace flowzone
