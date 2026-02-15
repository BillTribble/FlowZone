#include "CombFilter.h"
#include <algorithm>

namespace flowzone {
namespace dsp {

CombFilter::CombFilter() { updateInternalBuffer(); }

void CombFilter::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  updateInternalBuffer();
}

void CombFilter::process(juce::AudioBuffer<float> &buffer) {
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int numSamples = buffer.getNumSamples();

  for (int i = 0; i < numSamples; ++i) {
    // Output = delayed_sample
    float delayedL = delayBufferL[writeIndex];
    float delayedR = delayBufferR[writeIndex];

    // Apply damping: lastOut = lastOut * damp + delayed * (1 - damp)
    lastOutL = (delayedL * (1.0f - damping)) + (lastOutL * damping);
    lastOutR = (delayedR * (1.0f - damping)) + (lastOutR * damping);

    // Feedback loop
    delayBufferL[writeIndex] = left[i] + (lastOutL * feedback);
    delayBufferR[writeIndex] =
        (right ? right[i] : left[i]) + (lastOutR * feedback);

    // Write to output (replace or add? User spec says "Filter-based effects",
    // usually replace)
    left[i] = lastOutL;
    if (right)
      right[i] = lastOutR;

    // Advance index
    writeIndex = (writeIndex + 1) % (int)delayBufferL.size();
  }
}

void CombFilter::reset() {
  std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
  std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
  writeIndex = 0;
  lastOutL = 0.0f;
  lastOutR = 0.0f;
}

void CombFilter::setParameter(int index, float value) {
  // index 0: delay (0.1ms to 100ms)
  // index 1: feedback (0 to 0.99)
  // index 2: damping (0 to 1.0)
  if (index == 0) {
    setDelay(0.1f + value * 99.9f);
  } else if (index == 1) {
    setFeedback(value * 0.99f);
  } else if (index == 2) {
    setDamping(value);
  }
}

void CombFilter::setDelay(float ms) {
  delayMs = juce::jlimit(0.1f, 1000.0f,
                         ms); // Allow up to 1s internal, though spec says 100ms
  updateInternalBuffer();
}

void CombFilter::setFeedback(float fb) {
  feedback = juce::jlimit(0.0f, 0.99f, fb);
}

void CombFilter::setDamping(float damp) {
  damping = juce::jlimit(0.0f, 1.0f, damp);
}

void CombFilter::updateInternalBuffer() {
  int samples = (int)(currentSampleRate * (delayMs / 1000.0));
  samples = std::max(1, samples);

  if (samples != (int)delayBufferL.size()) {
    delayBufferL.resize(samples, 0.0f);
    delayBufferR.resize(samples, 0.0f);
    writeIndex = writeIndex % samples;
  }
}

} // namespace dsp
} // namespace flowzone
