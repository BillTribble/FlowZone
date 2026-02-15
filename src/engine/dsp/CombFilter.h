#pragma once

#include "FilterBase.h"
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Feedback Comb Filter using a internal delay buffer.
 */
class CombFilter : public FilterBase {
public:
  CombFilter();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setDelay(float delayMs);
  void setFeedback(float feedback);
  void setDamping(float damping);

private:
  double currentSampleRate = 44100.0;
  float delayMs = 10.0f;
  float feedback = 0.5f;
  float damping = 0.2f;

  std::vector<float> delayBufferL;
  std::vector<float> delayBufferR;
  int writeIndex = 0;
  float lastOutL = 0.0f;
  float lastOutR = 0.0f;

  void updateInternalBuffer();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombFilter)
};

} // namespace dsp
} // namespace flowzone
