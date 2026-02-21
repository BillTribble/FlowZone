#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * High-performance FX engine using JUCE DSP library.
 * Provides Delay and Reverb modules that can be selectively applied.
 */
class StandaloneFXEngine {
public:
  StandaloneFXEngine() {
    // Basic initialization
  }

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    delayLine.prepare(spec);
    reverb.prepare(spec);

    // Initialize delay line maximum duration (e.g. 2 seconds)
    delayLine.setMaximumDelayInSamples(static_cast<int>(spec.sampleRate * 2.0));
  }

  void setDelayParams(float timeSeconds, float feedback) {
    activeDelayTime = timeSeconds;
    activeFeedback = feedback;
  }

  void setReverbParams(float roomSize, float wetLevel) {
    reverbParams.roomSize = roomSize;
    reverbParams.wetLevel = wetLevel;
    reverb.setParameters(reverbParams);
  }

  /**
   * Process an audio block through the FX chain.
   * This is intended for the "Isolator" buffer (selected layers).
   */
  void process(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();

    // --- 1. Delay Processing ---
    for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
      auto *samples = outBlock.getChannelPointer(ch);
      float delayInSamples = activeDelayTime * (float)sampleRate;

      for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
        float input = samples[i];
        float delayed = delayLine.popSample((int)ch, delayInSamples);

        // Simple feedback loop
        delayLine.pushSample((int)ch, input + delayed * activeFeedback);

        // Mix wet result (full wet for the isolator, MainComponent handles dry
        // mix)
        samples[i] = input + delayed;
      }
    }

    // --- 2. Reverb Processing ---
    reverb.process(context);
  }

  void reset() {
    delayLine.reset();
    reverb.reset();
  }

  void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

private:
  juce::dsp::DelayLine<float> delayLine;
  juce::dsp::Reverb reverb;
  juce::dsp::Reverb::Parameters reverbParams;

  float activeDelayTime = 0.5f;
  float activeFeedback = 0.3f;
  double sampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneFXEngine)
};
