#include "DistortionEffect.h"
#include <algorithm>
#include <cmath>

namespace flowzone {
namespace dsp {

DistortionEffect::DistortionEffect() {}

void DistortionEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void DistortionEffect::process(juce::AudioBuffer<float> &buffer) {
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  for (int ch = 0; ch < numChannels; ++ch) {
    auto *data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      data[i] = applyDistortion(data[i]);
    }
  }
}

void DistortionEffect::reset() {}

void DistortionEffect::setParameter(int index, float value) {
  // 0: Drive (1.0 to 10.0)
  // 1: Folding (0.0 to 1.0)
  if (index == 0)
    setDrive(1.0f + value * 9.0f);
  else if (index == 1)
    setWavefold(value);
}

void DistortionEffect::setDrive(float d) { drive = d; }
void DistortionEffect::setWavefold(float f) { folding = f; }

float DistortionEffect::applyDistortion(float input) {
  float x = input * drive;

  // Wavefolding (reflection)
  if (folding > 0.01f) {
    float threshold = 1.0f - (folding * 0.8f);
    while (std::abs(x) > threshold) {
      if (x > threshold)
        x = threshold - (x - threshold);
      else if (x < -threshold)
        x = -threshold - (x + threshold);
    }
  }

  // Soft clipping (tanh)
  return std::tanh(x);
}

} // namespace dsp
} // namespace flowzone
