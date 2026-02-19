#include "BitcrusherEffect.h"
#include <cmath>

namespace flowzone {
namespace dsp {

BitcrusherEffect::BitcrusherEffect() {}

void BitcrusherEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate, samplesPerBlock);
  reset();
}

void BitcrusherEffect::process(juce::AudioBuffer<float> &buffer) {
  int numSamples = buffer.getNumSamples();
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

  float levels = std::pow(2.0f, bits);

  for (int i = 0; i < numSamples; ++i) {
    phase += 1.0f;

    if (phase >= rateDivider) {
      phase -= rateDivider;

      float inL = left[i];
      float inR = right ? right[i] : inL;

      // Bit depth reduction
      lastOutL = std::round(inL * levels) / levels;
      lastOutR = std::round(inR * levels) / levels;
    }

    left[i] = lastOutL;
    if (right)
      right[i] = lastOutR;
  }
}

void BitcrusherEffect::reset() {
  lastOutL = 0.0f;
  lastOutR = 0.0f;
  phase = 0.0f;
}

void BitcrusherEffect::setParameter(int index, float value) {
  // 0: Bits (1.0 to 16.0)
  // 1: Rate Divider (1.0 to 50.0)
  if (index == 0)
    setBits(1.0f + (1.0f - value) * 15.0f); // Map 0-1 to 16-1 bits
  else if (index == 1)
    setRateDivider(1.0f + value * 49.0f);
}

void BitcrusherEffect::setBits(float b) { bits = juce::jlimit(1.0f, 32.0f, b); }
void BitcrusherEffect::setRateDivider(float r) {
  rateDivider = juce::jlimit(1.0f, 100.0f, r);
}

} // namespace dsp
} // namespace flowzone
