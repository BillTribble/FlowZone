#include "CompressorEffect.h"

namespace flowzone {
namespace dsp {

CompressorEffect::CompressorEffect() {
  setThreshold(-20.0f);
  setRatio(4.0f);
  setAttack(10.0f);
  setRelease(100.0f);
}

void CompressorEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  compressor.prepare(spec);
}

void CompressorEffect::process(juce::AudioBuffer<float> &buffer) {
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  compressor.process(context);
}

void CompressorEffect::reset() { compressor.reset(); }

void CompressorEffect::setParameter(int index, float value) {
  // 0: threshold (0 to -60dB)
  // 1: ratio (1 to 20)
  // 2: attack (0.1 to 100ms)
  // 3: release (1 to 1000ms)
  if (index == 0)
    setThreshold(0.0f - value * 60.0f);
  else if (index == 1)
    setRatio(1.0f + value * 19.0f);
  else if (index == 2)
    setAttack(0.1f + value * 99.9f);
  else if (index == 3)
    setRelease(1.0f + value * 999.0f);
}

void CompressorEffect::setThreshold(float db) { compressor.setThreshold(db); }
void CompressorEffect::setRatio(float r) { compressor.setRatio(r); }
void CompressorEffect::setAttack(float ms) { compressor.setAttack(ms); }
void CompressorEffect::setRelease(float ms) { compressor.setRelease(ms); }

} // namespace dsp
} // namespace flowzone
