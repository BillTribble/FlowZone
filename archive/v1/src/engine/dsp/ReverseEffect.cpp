#include "ReverseEffect.h"
#include <algorithm>

namespace flowzone {
namespace dsp {

ReverseEffect::ReverseEffect() {
  bufferL.resize(bufferSize, 0.0f);
  bufferR.resize(bufferSize, 0.0f);
}

void ReverseEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  bufferSize = (int)sampleRate; // 1s reverse buffer
  bufferL.resize(bufferSize, 0.0f);
  bufferR.resize(bufferSize, 0.0f);
  reset();
}

void ReverseEffect::process(juce::AudioBuffer<float> &buffer) {
  int numSamples = buffer.getNumSamples();
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    float inL = left[i];
    float inR = right ? right[i] : inL;

    // Continuous capture
    bufferL[writeIndex] = inL;
    bufferR[writeIndex] = inR;
    writeIndex = (writeIndex + 1) % bufferSize;

    if (isEnabled) {
      // Play backwards relative to write head
      left[i] = bufferL[readIndex];
      if (right)
        right[i] = bufferR[readIndex];

      readIndex--;
      if (readIndex < 0)
        readIndex = bufferSize - 1;
    } else {
      readIndex = writeIndex; // Keep read index pinned to the present
    }
  }
}

void ReverseEffect::reset() {
  std::fill(bufferL.begin(), bufferL.end(), 0.0f);
  std::fill(bufferR.begin(), bufferR.end(), 0.0f);
  writeIndex = 0;
  readIndex = 0;
}

void ReverseEffect::setParameter(int index, float value) {
  if (index == 0)
    setEnabled(value > 0.5f);
}

void ReverseEffect::setEnabled(bool e) { isEnabled = e; }

} // namespace dsp
} // namespace flowzone
