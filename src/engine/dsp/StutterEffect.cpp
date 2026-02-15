#include "StutterEffect.h"
#include <algorithm>

namespace flowzone {
namespace dsp {

StutterEffect::StutterEffect() { updateInternalBuffer(); }

void StutterEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  // currentSampleRate = sampleRate; // Add if needed
  updateInternalBuffer();
}

void StutterEffect::process(juce::AudioBuffer<float> &buffer) {
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int numSamples = buffer.getNumSamples();

  if (!isEnabled) {
    // Just capture into the buffer continuously
    for (int i = 0; i < numSamples; ++i) {
      stutterBufferL[writeIndex] = left[i];
      stutterBufferR[writeIndex] = (right ? right[i] : left[i]);
      writeIndex = (writeIndex + 1) % (int)stutterBufferL.size();
    }
    readIndex = writeIndex; // Keep read index aligned for immediate start
    return;
  }

  // Stuttering: read from buffer repeatedly
  for (int i = 0; i < numSamples; ++i) {
    left[i] = stutterBufferL[readIndex];
    if (right)
      right[i] = stutterBufferR[readIndex];

    readIndex = (readIndex + 1) % (int)stutterBufferL.size();
  }
}

void StutterEffect::reset() {
  std::fill(stutterBufferL.begin(), stutterBufferL.end(), 0.0f);
  std::fill(stutterBufferR.begin(), stutterBufferR.end(), 0.0f);
  readIndex = 0;
  writeIndex = 0;
}

void StutterEffect::setParameter(int index, float value) {
  // 0: Enabled (0 or 1)
  // 1: Size (1ms to 500ms)
  if (index == 0)
    setEnabled(value > 0.5f);
  else if (index == 1)
    setStutterSize(1.0f + value * 499.0f);
}

void StutterEffect::setEnabled(bool e) { isEnabled = e; }

void StutterEffect::setStutterSize(float ms) {
  if (std::abs(ms - stutterMs) > 0.1f) {
    stutterMs = ms;
    updateInternalBuffer();
  }
}

void StutterEffect::updateInternalBuffer() {
  int samples =
      (int)(44100.0 * (stutterMs / 1000.0)); // Assume 44.1k for sizing
  samples = std::max(64, samples);

  if (samples != (int)stutterBufferL.size()) {
    stutterBufferL.resize(samples, 0.0f);
    stutterBufferR.resize(samples, 0.0f);
    writeIndex = writeIndex % samples;
    readIndex = readIndex % samples;
  }
}

} // namespace dsp
} // namespace flowzone
