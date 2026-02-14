#include "FlowEngine.h"

namespace flowzone {

FlowEngine::FlowEngine() {}
FlowEngine::~FlowEngine() {}

void FlowEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  transport.prepareToPlay(sampleRate, samplesPerBlock);
}

void FlowEngine::processBlock(juce::AudioBuffer<float> &buffer,
                              juce::MidiBuffer &midiMessages) {
  transport.processBlock(buffer, midiMessages);
  // TODO: Process Loopers
}

} // namespace flowzone
