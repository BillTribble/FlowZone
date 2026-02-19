#include "LimiterEffect.h"

namespace flowzone {
namespace dsp {

LimiterEffect::LimiterEffect() {
  setThreshold(0.0f);
  setRelease(100.0f);
}

void LimiterEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  limiter.prepare(spec);
}

void LimiterEffect::process(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  limiter.process(context);
}

void LimiterEffect::reset() { limiter.reset(); }

void LimiterEffect::setParameter(int index, float value) {
  // 0: threshold (0 to -24dB)
  // 1: release (1 to 500ms)
  if (index == 0)
    setThreshold(0.0f - value * 24.0f);
  else if (index == 1)
    setRelease(1.0f + value * 499.0f);
}

void LimiterEffect::setThreshold(float db) { limiter.setThreshold(db); }
void LimiterEffect::setRelease(float ms) { limiter.setRelease(ms); }

} // namespace dsp
} // namespace flowzone
