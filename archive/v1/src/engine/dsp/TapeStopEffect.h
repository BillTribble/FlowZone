#pragma once

#include "FilterBase.h"
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Tape Stop effect: Gradually slows down playback speed.
 */
class TapeStopEffect : public FilterBase {
public:
  TapeStopEffect();

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  void setStop(bool stopping);

private:
  bool isStopping = false;
  float currentSpeed = 1.0f;
  float stopRate = 0.001f;

  std::vector<float> historyBufferL;
  std::vector<float> historyBufferR;
  int writeIndex = 0;
  float readPhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeStopEffect)
};

} // namespace dsp
} // namespace flowzone
