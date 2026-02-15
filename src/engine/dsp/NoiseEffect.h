#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Noise generator with filtering.
 * Can be used as a sound source or an effect (additive).
 */
class NoiseEffect : public FilterBase {
public:
  NoiseEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setMix(float mix);
  void setCutoff(float hz);

private:
  float mix = 0.2f;
  juce::Random random;
  juce::dsp::IIR::Filter<float> filter;
  double currentSampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseEffect)
};

} // namespace dsp
} // namespace flowzone
