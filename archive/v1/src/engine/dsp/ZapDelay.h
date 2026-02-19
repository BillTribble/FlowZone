#pragma once

#include "DelayEffect.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Zap Delay: Modulated delay for sci-fi pitch artifacts and chorus.
 */
class ZapDelay : public DelayEffect {
public:
  ZapDelay();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void setParameter(int index, float value) override;

  void setModDepth(float depth);
  void setModRate(float rateHz);

private:
  float modDepth = 0.5f;
  float modRate = 2.0f;
  float modPhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZapDelay)
};

} // namespace dsp
} // namespace flowzone
