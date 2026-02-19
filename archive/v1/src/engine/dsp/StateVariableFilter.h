#pragma once

#include "FilterBase.h"

namespace flowzone {
namespace dsp {

/**
 * @brief State Variable Filter (SVF) implementation for Lowpass and Highpass.
 * Stable for parameter modulation and predictable phase response.
 */
class StateVariableFilter : public FilterBase {
public:
  enum Type { Lowpass, Highpass };

  StateVariableFilter(Type type = Lowpass);

  void prepare(double sampleRate, int samplesPerBlock) override;
  void process(juce::AudioBuffer<float> &buffer) override;
  void reset() override;
  void setParameter(int index, float value) override;

  /**
   * @brief Set cutoff frequency in Hz.
   */
  void setCutoff(float frequencyHz);

  /**
   * @brief Set Resonance (Q value).
   */
  void setResonance(float resonance);

private:
  juce::dsp::StateVariableTPTFilter<float> filter;
  Type filterType;
  double currentSampleRate = 44100.0;
  float cutoff = 1000.0f;
  float resonance = 0.707f;

  void updateFilter();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateVariableFilter)
};

} // namespace dsp
} // namespace flowzone
