#include "DubDelay.h"
#include <cmath>

namespace flowzone {
namespace dsp {

DubDelay::DubDelay() : DelayEffect() {
  delayMs = 300.0f;
  feedback = 0.7f;
}

void DubDelay::prepare(double sampleRate, int samplesPerBlock) {
  DelayEffect::prepare(sampleRate, samplesPerBlock);
  lastOutL = 0.0f;
  lastOutR = 0.0f;
}

void DubDelay::process(juce::AudioBuffer<float> &buffer) {
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int numSamples = buffer.getNumSamples();

  float delayInSamples = (float)(currentSampleRate * (delayMs / 1000.0));
  int bufferSize = (int)delayBufferL.size();

  for (int i = 0; i < numSamples; ++i) {
    float inL = left[i];
    float inR = right ? right[i] : inL;

    float delayedL = getDelayedSample(0, delayInSamples);
    float delayedR = getDelayedSample(1, delayInSamples);

    // Feedback Damping (simple LPF)
    lastOutL = (delayedL * (1.0f - damping)) + (lastOutL * damping);
    lastOutR = (delayedR * (1.0f - damping)) + (lastOutR * damping);

    // Saturation
    float feedbackL = applySaturation(lastOutL * feedback);
    float feedbackR = applySaturation(lastOutR * feedback);

    delayBufferL[writeIndex] = inL + feedbackL;
    delayBufferR[writeIndex] = inR + feedbackR;

    left[i] = (lastOutL * mix) + (inL * (1.0f - mix));
    if (right)
      right[i] = (lastOutR * mix) + (inR * (1.0f - mix));

    writeIndex = (writeIndex + 1) % bufferSize;
  }
}

void DubDelay::setParameter(int index, float value) {
  if (index < 3)
    DelayEffect::setParameter(index, value);
  else if (index == 3)
    setDamping(value);
  else if (index == 4)
    setSaturation(value);
}

void DubDelay::setDamping(float d) { damping = juce::jlimit(0.0f, 0.99f, d); }
void DubDelay::setSaturation(float s) {
  saturation = juce::jlimit(0.0f, 2.0f, s);
}

float DubDelay::applySaturation(float input) {
  // Simple soft clipping (hyperbolic tangent approximation or soft sigmoid)
  if (saturation <= 0.01f)
    return input;

  float drive = 1.0f + saturation * 2.0f;
  return std::tanh(input * drive) / std::tanh(drive);
}

} // namespace dsp
} // namespace flowzone
