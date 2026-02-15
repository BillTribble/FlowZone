#pragma once

#include <JuceHeader.h>

namespace flowzone {
namespace engine {

/**
 * @brief Polyphonic Synth Voice logic.
 */
class SynthVoice : public juce::SynthesiserVoice {
public:
  SynthVoice();

  bool canPlaySound(juce::SynthesiserSound *sound) override;
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;
  void pitchWheelMoved(int newPitchWheelValue) override;
  void controllerMoved(int controllerNumber, int newControllerValue) override;

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  // Parameter access
  void setOscillatorType(int type); // 0: sine, 1: saw, 2: square, 3: tri
  void setADSR(float a, float d, float s, float r);
  void setPitchRatio(float ratio);

private:
  double currentSampleRate = 44100.0;
  float level = 0.0f;
  float angleDelta = 0.0f;
  float currentAngle = 0.0f;
  int oscType = 1; // Default to Saw
  float pitchRatio = 1.0f;

  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;

  // Temporary: simple Sine/Saw implementation
  float getNextSample();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthVoice)
};

} // namespace engine
} // namespace flowzone
