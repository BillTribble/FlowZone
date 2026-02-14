#pragma once
#include "transport/TransportService.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-3uw: FlowEngine Skeleton
// Coordinates DSP graph, loopers, and transport
class FlowEngine {
public:
  FlowEngine();
  ~FlowEngine();

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages);

  TransportService &getTransport() { return transport; }

private:
  TransportService transport;
  // TODO: Add LooperGraph, effects chain
};

} // namespace flowzone
