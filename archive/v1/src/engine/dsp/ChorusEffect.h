#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Chorus effect.
 */
class ChorusEffect : public FilterBase {
public:
  ChorusEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setRate(float hz);
  void setDepth(float depth);
  void setCentreDelay(float ms);
  void setFeedback(float fb);
  void setMix(float mix);

private:
  juce::dsp::Chorus<float> chorus;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusEffect)
};

} // namespace dsp
} // namespace flowzone
