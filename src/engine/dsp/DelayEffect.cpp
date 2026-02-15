#include "DelayEffect.h"
#include <algorithm>

namespace flowzone {
namespace dsp {

DelayEffect::DelayEffect() { updateBuffer(); }

void DelayEffect::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  updateBuffer();
}

void DelayEffect::process(juce::AudioBuffer<float> &buffer) {
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int numSamples = buffer.getNumSamples();

  float delayInSamples = (float)(currentSampleRate * (delayMs / 1000.0));
  int bufferSize = (int)delayBufferL.size();

  for (int i = 0; i < numSamples; ++i) {
    float inL = left[i];
    float inR = right ? right[i] : inL;

    // Read from delay line (fractional for smoothness)
    float delayedL = getDelayedSample(0, delayInSamples);
    float delayedR = getDelayedSample(1, delayInSamples);

    // Feedback
    delayBufferL[writeIndex] = inL + (delayedL * feedback);
    delayBufferR[writeIndex] = inR + (delayedR * feedback);

    // Mix: wet * mix + dry * (1 - mix)
    left[i] = (delayedL * mix) + (inL * (1.0f - mix));
    if (right)
      right[i] = (delayedR * mix) + (inR * (1.0f - mix));

    // Advance write index
    writeIndex = (writeIndex + 1) % bufferSize;
  }
}

void DelayEffect::reset() {
  std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
  std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
}

void DelayEffect::setParameter(int index, float value) {
  // index 0: delay (0 to 2000ms)
  // index 1: feedback (0 to 1.0)
  // index 2: mix (0 to 1.0)
  if (index == 0)
    setDelayMs(value * 2000.0f);
  else if (index == 1)
    setFeedback(value);
  else if (index == 2)
    setMix(value);
}

void DelayEffect::setDelayMs(float ms) {
  delayMs = juce::jlimit(0.0f, 5000.0f, ms);
  updateBuffer();
}

void DelayEffect::setFeedback(float fb) {
  feedback = juce::jlimit(
      0.0f, 1.1f, fb); // Allow > 1 for intentional feedback loops in some modes
}

void DelayEffect::setMix(float m) { mix = juce::jlimit(0.0f, 1.0f, m); }

void DelayEffect::updateBuffer() {
  int maxSamples = (int)(currentSampleRate * 5.1); // 5s safety
  if ((int)delayBufferL.size() != maxSamples) {
    delayBufferL.resize(maxSamples, 0.0f);
    delayBufferR.resize(maxSamples, 0.0f);
    writeIndex = writeIndex % maxSamples;
  }
}

float DelayEffect::getDelayedSample(int channel, float delayInSamples) {
  int bufferSize = (int)delayBufferL.size();
  float readPos = (float)writeIndex - delayInSamples;

  while (readPos < 0)
    readPos += (float)bufferSize;

  int idx1 = (int)readPos % bufferSize;
  int idx2 = (idx1 + 1) % bufferSize;
  float frac = readPos - (float)idx1;

  const auto &buf = (channel == 0) ? delayBufferL : delayBufferR;
  return buf[idx1] * (1.0f - frac) + buf[idx2] * frac;
}

} // namespace dsp
} // namespace flowzone
