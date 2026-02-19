#pragma once

#include "DelayEffect.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Dub Delay: Delay with feedback damping (LPF) and saturation.
 * Emulates vintage tape eco/dub style.
 */
class DubDelay : public DelayEffect {
public:
  DubDelay();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void setParameter(int index, float value) override;

  void setDamping(float damping);
  void setSaturation(float saturation);

private:
  float damping = 0.5f;
  float saturation = 0.5f;
  float lastOutL = 0.0f;
  float lastOutR = 0.0f;

  float applySaturation(float input);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DubDelay)
};

} // namespace dsp
} // namespace flowzone
