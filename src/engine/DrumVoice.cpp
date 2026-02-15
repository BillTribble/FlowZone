#include "DrumVoice.h"
#include <cmath>

namespace flowzone {
namespace engine {

DrumVoice::DrumVoice() {}

void DrumVoice::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  juce::ignoreUnused(samplesPerBlock);
}

void DrumVoice::trigger(float velocity, Type type) {
  active = true;
  currentType = type;
  level = velocity;
  phase = 0.0f;
  envValue = 1.0f;

  switch (type) {
  case Type::Kick:
    phaseDelta = (float)(60.0 * 2.0 * juce::MathConstants<double>::pi /
                         currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.3); // 300ms
    pitchEnvValue = 1.0f;
    pitchEnvDelta = 1.0f / (float)(currentSampleRate * 0.05); // 50ms pitch drop
    break;
  case Type::Snare:
    envDelta = 1.0f / (float)(currentSampleRate * 0.2); // 200ms
    break;
  case Type::Hihat:
    envDelta = 1.0f / (float)(currentSampleRate * 0.05); // 50ms
    break;
  case Type::Perc:
    envDelta = 1.0f / (float)(currentSampleRate * 0.1); // 100ms
    break;
  }
}

void DrumVoice::process(juce::AudioBuffer<float> &buffer, int startSample,
                        int numSamples) {
  if (!active)
    return;

  switch (currentType) {
  case Type::Kick:
    processKick(buffer, startSample, numSamples);
    break;
  case Type::Snare:
    processSnare(buffer, startSample, numSamples);
    break;
  case Type::Hihat:
    processHihat(buffer, startSample, numSamples);
    break;
  case Type::Perc:
    processHihat(buffer, startSample, numSamples);
    break; // fallback
  }

  if (envValue <= 0.0f)
    active = false;
}

void DrumVoice::processKick(juce::AudioBuffer<float> &buffer, int start,
                            int n) {
  for (int i = 0; i < n; ++i) {
    float freqMult = 1.0f + pitchEnvValue * 2.0f;
    float s = std::sin(phase * freqMult) * envValue * level;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta;
    envValue -= envDelta;
    pitchEnvValue = std::max(0.0f, pitchEnvValue - pitchEnvDelta);
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processSnare(juce::AudioBuffer<float> &buffer, int start,
                             int n) {
  for (int i = 0; i < n; ++i) {
    float noise = (random.nextFloat() * 2.0f - 1.0f) * envValue * level;
    float tone = std::sin(phase) * envValue * level * 0.3f;
    float s = (noise + tone) * 0.7f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta * 2.0f;
    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processHihat(juce::AudioBuffer<float> &buffer, int start,
                             int n) {
  for (int i = 0; i < n; ++i) {
    float s = (random.nextFloat() * 2.0f - 1.0f) * envValue * level * 0.5f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

} // namespace engine
} // namespace flowzone
