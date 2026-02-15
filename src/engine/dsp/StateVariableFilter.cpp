#include "StateVariableFilter.h"

namespace flowzone {
namespace dsp {

StateVariableFilter::StateVariableFilter(Type type) : filterType(type) {
  updateFilter();
}

void StateVariableFilter::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2; // FlowZone is stereo-first

  filter.prepare(spec);
  updateFilter();
}

void StateVariableFilter::process(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);
}

void StateVariableFilter::reset() { filter.reset(); }

void StateVariableFilter::setParameter(int index, float value) {
  // index 0: cutoff (normalized 0-1)
  // index 1: resonance (normalized 0-1)
  if (index == 0) {
    // Map 20Hz - 20kHz exponentially
    float freq = 20.0f * std::pow(1000.0f, value);
    setCutoff(freq);
  } else if (index == 1) {
    // Map 0.1 - 10.0
    setResonance(0.1f + value * 9.9f);
  }
}

void StateVariableFilter::setCutoff(float frequencyHz) {
  cutoff = juce::jlimit(20.0f, 20000.0f, frequencyHz);
  updateFilter();
}

void StateVariableFilter::setResonance(float res) {
  resonance = juce::jlimit(0.1f, 10.0f, res);
  updateFilter();
}

void StateVariableFilter::updateFilter() {
  if (filterType == Lowpass)
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  else
    filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);

  filter.setCutoffFrequency(cutoff);
  filter.setResonance(resonance);
}

} // namespace dsp
} // namespace flowzone
