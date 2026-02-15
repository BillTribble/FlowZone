#pragma once

#include "CombFilter.h"
#include <memory>
#include <vector>

namespace flowzone {
namespace dsp {

/**
 * @brief Multicomb filter: Parallel bank of CombFilters.
 * Used for dense resonances and reverb-like textures.
 */
class MulticombFilter : public FilterBase {
public:
  MulticombFilter(int numCombs = 4);

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  /**
   * @brief Set base delay and spread for the bank.
   */
  void setBankParams(float baseDelayMs, float spread, float feedback);

private:
  std::vector<std::unique_ptr<CombFilter>> combs;
  juce::AudioBuffer<float> accumulationBuffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MulticombFilter)
};

} // namespace dsp
} // namespace flowzone
