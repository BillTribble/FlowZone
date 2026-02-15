#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Brickwall Limiter effect.
 */
class LimiterEffect : public FilterBase {
public:
  LimiterEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setThreshold(float db);
  void setRelease(float ms);

private:
  juce::dsp::Limiter<float> limiter;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterEffect)
};

} // namespace dsp
} // namespace flowzone
