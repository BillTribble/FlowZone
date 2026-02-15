#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Bitcrusher: Sample rate and bit depth reduction.
 */
class BitcrusherEffect : public FilterBase {
public:
  BitcrusherEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setBits(float bits);
  void setRateDivider(float divider);

private:
  float bits = 16.0f;
  float rateDivider = 1.0f;
  float lastOutL = 0.0f;
  float lastOutR = 0.0f;
  float phase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BitcrusherEffect)
};

} // namespace dsp
} // namespace flowzone
