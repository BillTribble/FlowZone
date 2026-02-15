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
    dispatcher.dispatch(commandStr);
  }
}

} // namespace flowzone
