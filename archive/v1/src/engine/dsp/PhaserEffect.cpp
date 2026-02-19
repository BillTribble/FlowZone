#include "PhaserEffect.h"

namespace flowzone {
namespace dsp {

PhaserEffect::PhaserEffect() {
  setRate(0.5f);
  setDepth(0.5f);
  setCentreFrequency(1000.0f);
  setFeedback(0.0f);
  setMix(0.5f);
}

void PhaserEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  phaser.prepare(spec);
}

void PhaserEffect::process(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  phaser.process(context);
}

void PhaserEffect::reset() { phaser.reset(); }

void PhaserEffect::setParameter(int index, float value) {
  // 0: rate (0 to 5Hz)
  // 1: depth (0 to 1.0)
  // 2: mix (0 to 1.0)
  if (index == 0)
    setRate(value * 5.0f);
  else if (index == 1)
    setDepth(value);
  else if (index == 2)
    setMix(value);
}

void PhaserEffect::setRate(float hz) { phaser.setRate(hz); }
void PhaserEffect::setDepth(float d) { phaser.setDepth(d); }
void PhaserEffect::setCentreFrequency(float hz) {
  phaser.setCentreFrequency(hz);
}
void PhaserEffect::setFeedback(float fb) { phaser.setFeedback(fb); }
void PhaserEffect::setMix(float m) { phaser.setMix(m); }

} // namespace dsp
} // namespace flowzone
