#pragma once

#include "FilterBase.h"
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Standard Feedback Delay effect.
 */
class DelayEffect : public FilterBase {
public:
  DelayEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setDelayMs(float ms);
  void setFeedback(float fb);
  void setMix(float mix);

protected:
  double currentSampleRate = 44100.0;
  float delayMs = 500.0f;
  float feedback = 0.5f;
  float mix = 0.5f;

  std::vector<float> delayBufferL;
  std::vector<float> delayBufferR;
  int writeIndex = 0;

  virtual void updateBuffer();
  float getDelayedSample(int channel, float delayInSamples);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayEffect)
};

} // namespace dsp
} // namespace flowzone
