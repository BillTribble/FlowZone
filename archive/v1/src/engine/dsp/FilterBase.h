#pragma once

#include <JuceHeader.h>

namespace flowzone {
namespace dsp {

/**
 * @brief Base class for all DSP filters in FlowZone.
 */
class FilterBase {
public:
  virtual ~FilterBase() = default;

  /**
   * @brief Prepares the filter for processing.
   * @param sampleRate The session sample rate.
   * @param samplesPerBlock The expected block size.
   */
  virtual void prepare(double sampleRate, int samplesPerBlock) = 0;

  /**
   * @brief Processes an audio block through the filter.
   * @param buffer The audio buffer to process (in-place).
   */
  virtual void process(juce::AudioBuffer<float> &buffer) = 0;

  /**
   * @brief Resets the filter's internal state.
   */
  virtual void reset() = 0;

  /**
   * @brief Sets primary filter parameters.
   * Specific parameter mapping is handled by subclasses.
   */
  virtual void setParameter(int index, float value) = 0;
};

} // namespace dsp
} // namespace flowzone
