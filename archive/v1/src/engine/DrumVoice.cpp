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
    phaseDelta = (float)(200.0 * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.2); // 200ms
    break;
  case Type::Hihat:
    envDelta = 1.0f / (float)(currentSampleRate * 0.05); // 50ms
    break;
  case Type::TomLow:
    baseFrequency = 80.0f; // Low tom
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.4); // 400ms
    pitchEnvValue = 1.0f;
    pitchEnvDelta = 1.0f / (float)(currentSampleRate * 0.08); // 80ms pitch drop
    break;
  case Type::TomMid:
    baseFrequency = 120.0f; // Mid tom
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.35); // 350ms
    pitchEnvValue = 1.0f;
    pitchEnvDelta = 1.0f / (float)(currentSampleRate * 0.07); // 70ms pitch drop
    break;
  case Type::TomHigh:
    baseFrequency = 180.0f; // High tom
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.3); // 300ms
    pitchEnvValue = 1.0f;
    pitchEnvDelta = 1.0f / (float)(currentSampleRate * 0.06); // 60ms pitch drop
    break;
  case Type::Clap:
    envDelta = 1.0f / (float)(currentSampleRate * 0.15); // 150ms
    break;
  case Type::Rim:
    baseFrequency = 800.0f;
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.03); // 30ms - sharp click
    break;
  case Type::Cowbell:
    baseFrequency = 540.0f;
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.2); // 200ms
    break;
  case Type::Clave:
    baseFrequency = 2500.0f; // High pitched click
    phaseDelta = (float)(baseFrequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate);
    envDelta = 1.0f / (float)(currentSampleRate * 0.02); // 20ms - very short
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
  case Type::TomLow:
    processTom(buffer, startSample, numSamples, 80.0f);
    break;
  case Type::TomMid:
    processTom(buffer, startSample, numSamples, 120.0f);
    break;
  case Type::TomHigh:
    processTom(buffer, startSample, numSamples, 180.0f);
    break;
  case Type::Clap:
    processClap(buffer, startSample, numSamples);
    break;
  case Type::Rim:
    processRim(buffer, startSample, numSamples);
    break;
  case Type::Cowbell:
    processCowbell(buffer, startSample, numSamples);
    break;
  case Type::Clave:
    processClave(buffer, startSample, numSamples);
    break;
  case Type::Perc:
    processHihat(buffer, startSample, numSamples);
    break;
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

void DrumVoice::processTom(juce::AudioBuffer<float> &buffer, int start,
                           int n, float baseFreq) {
  for (int i = 0; i < n; ++i) {
    // Tom: pitched sine with pitch envelope + small noise component for realism
    float freqMult = 1.0f + pitchEnvValue * 1.5f; // Less pitch drop than kick
    float tone = std::sin(phase * freqMult) * envValue * level * 0.8f;
    float noise = (random.nextFloat() * 2.0f - 1.0f) * envValue * level * 0.1f;
    float s = tone + noise;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta;
    envValue -= envDelta;
    pitchEnvValue = std::max(0.0f, pitchEnvValue - pitchEnvDelta);
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processClap(juce::AudioBuffer<float> &buffer, int start,
                            int n) {
  // Clap: Multiple burst of noise with slight delay
  for (int i = 0; i < n; ++i) {
    float s = (random.nextFloat() * 2.0f - 1.0f) * envValue * level * 0.6f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processRim(juce::AudioBuffer<float> &buffer, int start,
                           int n) {
  // Rimshot: Very short, high-freq click with harmonics
  for (int i = 0; i < n; ++i) {
    float tone1 = std::sin(phase) * envValue * level * 0.5f;
    float tone2 = std::sin(phase * 2.4f) * envValue * level * 0.3f;
    float click = (random.nextFloat() * 2.0f - 1.0f) * envValue * level * 0.2f;
    float s = tone1 + tone2 + click;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta;
    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processCowbell(juce::AudioBuffer<float> &buffer, int start,
                               int n) {
  // Cowbell: Dual-tone metallic sound
  for (int i = 0; i < n; ++i) {
    float tone1 = std::sin(phase) * envValue * level * 0.4f;
    float tone2 = std::sin(phase * 1.5f) * envValue * level * 0.3f;
    float s = (tone1 + tone2) * 0.7f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta;
    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

void DrumVoice::processClave(juce::AudioBuffer<float> &buffer, int start,
                             int n) {
  // Clave: Very short, high-pitched click
  for (int i = 0; i < n; ++i) {
    float s = std::sin(phase) * envValue * level * 0.6f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.addSample(ch, start + i, s);

    phase += phaseDelta;
    envValue -= envDelta;
    if (envValue <= 0)
      break;
  }
}

} // namespace engine
} // namespace flowzone
