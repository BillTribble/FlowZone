#pragma once

#include <JuceHeader.h>

namespace flowzone {
namespace engine {

/**
 * @brief Mic Processor handling input gain, monitoring, and built-in reverb.
 */
class MicProcessor {
public:
  MicProcessor();

  void prepare(double sampleRate, int samplesPerBlock);
  void process(const juce::AudioBuffer<float> &inputBuffer,
               juce::AudioBuffer<float> &outputBuffer);
  void reset();

  // Parameters
  void setInputGain(float gainDb); // -60 to +40
  void setMonitorEnabled(bool enabled);
  void setReverbLevel(float level); // 0 to 1
  void setMonitorUntilLooped(bool enabled);

private:
  float inputGain = 1.0f; // Linear gain
  bool monitorEnabled = false;
  bool monitorUntilLooped = false;

  juce::Reverb reverb;
  juce::Reverb::Parameters reverbParams;
  float reverbLevel = 0.0f;

  juce::AudioBuffer<float> internalBuffer;

  void applyGain(juce::AudioBuffer<float> &buffer);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicProcessor)
};

} // namespace engine
} // namespace flowzone
