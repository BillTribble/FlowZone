#include "ZapDelay.h"
#include <cmath>

namespace flowzone {
namespace dsp {

ZapDelay::ZapDelay() : DelayEffect() {
  delayMs = 20.0f; // Default for chorus/flange zap
}

void ZapDelay::prepare(double sampleRate, int samplesPerBlock) {
  DelayEffect::prepare(sampleRate, samplesPerBlock);
  modPhase = 0.0f;
}

void ZapDelay::process(juce::AudioBuffer<float> &buffer) {
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int numSamples = buffer.getNumSamples();

  float baseDelaySamples = (float)(currentSampleRate * (delayMs / 1000.0));
  float modDepthSamples = (float)(currentSampleRate * (modDepth / 1000.0));
  int bufferSize = (int)delayBufferL.size();

  for (int i = 0; i < numSamples; ++i) {
    float inL = left[i];
    float inR = right ? right[i] : inL;

    // Modulate delay: LFO is a sine wave
    float lfo = std::sin(modPhase * 2.0f * juce::MathConstants<float>::pi);
    float modulatedDelay = baseDelaySamples + (lfo * modDepthSamples);

    // Ensure delay is always positive and within reasonable bounds
    modulatedDelay = std::max(1.0f, modulatedDelay);

    float delayedL = getDelayedSample(0, modulatedDelay);
    float delayedR = getDelayedSample(1, modulatedDelay);

    delayBufferL[writeIndex] = inL + (delayedL * feedback);
    delayBufferR[writeIndex] = inR + (delayedR * feedback);

    left[i] = (delayedL * mix) + (inL * (1.0f - mix));
    if (right)
      right[i] = (delayedR * mix) + (inR * (1.0f - mix));

    // Advance
    writeIndex = (writeIndex + 1) % bufferSize;
    modPhase += (float)(modRate / currentSampleRate);
    if (modPhase > 1.0f)
      modPhase -= 1.0f;
  }
}

void ZapDelay::setParameter(int index, float value) {
  // 0: delay, 1: feedback, 2: mix (from base)
  // 3: mod rate (0 to 10Hz)
  // 4: mod depth (0 to 50ms)
  if (index < 3)
    DelayEffect::setParameter(index, value);
  else if (index == 3)
    setModRate(value * 10.0f);
  else if (index == 4)
    setModDepth(value * 50.0f);
}

void ZapDelay::setModDepth(float depth) { modDepth = depth; }
void ZapDelay::setModRate(float rate) { modRate = rate; }

} // namespace dsp
} // namespace flowzone
