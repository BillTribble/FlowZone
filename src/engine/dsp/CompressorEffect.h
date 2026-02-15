#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Dynamics Compression effect.
 */
class CompressorEffect : public FilterBase {
public:
  CompressorEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setThreshold(float db);
  void setRatio(float ratio);
  void setAttack(float ms);
  void setRelease(float ms);

private:
  juce::dsp::Compressor<float> compressor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorEffect)
};

} // namespace dsp
} // namespace flowzone
