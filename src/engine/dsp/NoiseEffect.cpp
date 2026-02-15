#include "NoiseEffect.h"

namespace flowzone {
namespace dsp {

NoiseEffect::NoiseEffect() { setCutoff(2000.0f); }

void NoiseEffect::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  filter.prepare(spec);
  setCutoff(2000.0f);
}

void NoiseEffect::process(juce::AudioBuffer<float> &buffer) {
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  for (int i = 0; i < numSamples; ++i) {
    float white = random.nextFloat() * 2.0f - 1.0f;

    // Filter the noise (mono for simplicity in gen)
    float filtered = filter.processSample(white);

    for (int ch = 0; ch < numChannels; ++ch) {
      float dry = buffer.getSample(ch, i);
      buffer.setSample(ch, i, dry + (filtered * mix));
    }
  }
}

void NoiseEffect::reset() { filter.reset(); }

void NoiseEffect::setParameter(int index, float value) {
  // 0: Mix (0 to 1.0)
  // 1: Cutoff (20Hz to 15kHz)
  if (index == 0)
    setMix(value);
  else if (index == 1)
    setCutoff(20.0f + value * 14980.0f);
}

void NoiseEffect::setMix(float m) { mix = m; }

void NoiseEffect::setCutoff(float hz) {
  auto coefficients =
      juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, hz);
  filter.coefficients = coefficients;
}

} // namespace dsp
} // namespace flowzone
