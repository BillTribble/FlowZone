#pragma once

#include "SynthSound.h"
#include "SynthVoice.h"
#include <JuceHeader.h>

namespace flowzone {
namespace engine {

/**
 * @brief Main Synth Engine managing polyphony and presets.
 */
class SynthEngine {
public:
  SynthEngine();

  void prepare(double sampleRate, int samplesPerBlock);
  void process(juce::AudioBuffer<float> &buffer,
               juce::MidiBuffer &midiMessages);
  void reset();

  void setPreset(const juce::String &category, const juce::String &presetId);
  void setParameter(int index, float value);
  void setGlobalPitchRatio(float ratio);

private:
  juce::Synthesiser synth;
  int maxVoices = 16;

  void setupVoices();
  void applyPreset(int oscType, float attack, float decay, float sustain,
                   float release);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthEngine)
};

} // namespace engine
} // namespace flowzone
