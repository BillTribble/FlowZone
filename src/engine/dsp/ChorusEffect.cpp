#include "ChorusEffect.h"

namespace flowzone {
namespace dsp {

ChorusEffect::ChorusEffect() {
  setRate(1.0f);
  setDepth(0.5f);
  setCentreDelay(7.0f);
  setFeedback(0.0f);
  setMix(0.5f);
}

void ChorusEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  chorus.prepare(spec);
}

void ChorusEffect::process(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  chorus.process(context);
}

void ChorusEffect::reset() { chorus.reset(); }

void ChorusEffect::setParameter(int index, float value) {
  // 0: rate (0 to 10Hz)
  // 1: depth (0 to 1.0)
  // 2: mix (0 to 1.0)
  if (index == 0)
    setRate(value * 10.0f);
  else if (index == 1)
    setDepth(value);
  else if (index == 2)
    setMix(value);
}

void ChorusEffect::setRate(float hz) { chorus.setRate(hz); }
void ChorusEffect::setDepth(float d) { chorus.setDepth(d); }
void ChorusEffect::setCentreDelay(float ms) { chorus.setCentreDelay(ms); }
void ChorusEffect::setFeedback(float fb) { chorus.setFeedback(fb); }
void ChorusEffect::setMix(float m) { chorus.setMix(m); }

} // namespace dsp
} // namespace flowzone
