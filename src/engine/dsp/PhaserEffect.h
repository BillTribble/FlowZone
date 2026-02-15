#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief Phaser effect.
 */
class PhaserEffect : public FilterBase {
public:
  PhaserEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setRate(float hz);
  void setDepth(float depth);
  void setCentreFrequency(float hz);
  void setFeedback(float fb);
  void setMix(float mix);

private:
  juce::dsp::Phaser<float> phaser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserEffect)
};

} // namespace dsp
} // namespace flowzone
