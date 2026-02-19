#pragma once

#include "FilterBase.h"
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Stutter effect: Repeatedly plays a small buffer slice.
 */
class StutterEffect : public FilterBase {
public:
  StutterEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setStutterSize(float ms);
  void setEnabled(bool enabled);

private:
  float stutterMs = 50.0f;
  bool isEnabled = false;

  std::vector<float> stutterBufferL;
  std::vector<float> stutterBufferR;
  int readIndex = 0;
  int writeIndex = 0;
  int captureCount = 0;
  bool isBufferFull = false;

  void updateInternalBuffer();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StutterEffect)
};

} // namespace dsp
} // namespace flowzone
