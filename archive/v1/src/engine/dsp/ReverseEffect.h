#pragma once

#include "FilterBase.h"
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Reverse effect: Plays the recently captured audio buffer backwards.
 */
class ReverseEffect : public FilterBase {
public:
  ReverseEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setEnabled(bool enabled);

private:
  bool isEnabled = false;
  std::vector<float> bufferL;
  std::vector<float> bufferR;
  int writeIndex = 0;
  int readIndex = 0;
  int bufferSize = 44100;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseEffect)
};

} // namespace dsp
} // namespace flowzone
