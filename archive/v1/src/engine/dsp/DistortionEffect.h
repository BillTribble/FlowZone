#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Distortion effect with Saturation and Wavefolding.
 */
class DistortionEffect : public FilterBase {
public:
  DistortionEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setDrive(float drive);
  void setWavefold(float folding);

private:
  float drive = 1.0f;
  float folding = 0.0f;

  float applyDistortion(float input);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionEffect)
};

} // namespace dsp
} // namespace flowzone
