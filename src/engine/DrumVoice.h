#pragma once

#include <JuceHeader.h>

namespace flowzone {
namespace engine {

/**
 * @brief Synthesis-based Drum Voice.
 * Supports Kick, Snare, Hi-hat, and Percussion.
 */
class DrumVoice {
public:
  enum class Type { Kick, Snare, Hihat, Perc };

  DrumVoice();

  void prepare(double sampleRate, int samplesPerBlock);
  void trigger(float velocity, Type type);
  void process(juce::AudioBuffer<float> &buffer, int startSample,
               int numSamples);
  bool isActive() const { return active; }

private:
  double currentSampleRate = 44100.0;
  bool active = false;
  Type currentType = Type::Kick;
  float level = 0.0f;

  // Synthesis state
  float phase = 0.0f;
  float phaseDelta = 0.0f;
  float envValue = 0.0f;
  float envDelta = 0.0f;
  float pitchEnvValue = 1.0f;
  float pitchEnvDelta = 0.0f;

  juce::Random random;

  void processKick(juce::AudioBuffer<float> &buffer, int start, int n);
  void processSnare(juce::AudioBuffer<float> &buffer, int start, int n);
  void processHihat(juce::AudioBuffer<float> &buffer, int start, int n);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumVoice)
};

} // namespace engine
} // namespace flowzone
