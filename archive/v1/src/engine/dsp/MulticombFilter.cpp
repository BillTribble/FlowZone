#include "MulticombFilter.h"

namespace flowzone {
namespace dsp {

MulticombFilter::MulticombFilter(int numCombs) {
  for (int i = 0; i < numCombs; ++i) {
    combs.push_back(std::make_unique<CombFilter>());
  }
}

void MulticombFilter::prepare(double sampleRate, int samplesPerBlock) {
  for (auto &comb : combs) {
    comb->prepare(sampleRate, samplesPerBlock);
  }
  accumulationBuffer.setSize(2, samplesPerBlock);
}

void MulticombFilter::process(juce::AudioBuffer<float> &buffer) {
  int numSamples = buffer.getNumSamples();
  accumulationBuffer.setSize(buffer.getNumChannels(), numSamples, false, true,
                             true);
  accumulationBuffer.clear();

  // Sum outputs of all parallel comb filters
  for (auto &comb : combs) {
    juce::AudioBuffer<float> tempBuffer;
    tempBuffer.makeCopyOf(buffer); // Input is the same for all
    comb->process(tempBuffer);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      accumulationBuffer.addFrom(ch, 0, tempBuffer, ch, 0, numSamples,
                                 1.0f / combs.size());
    }
  }

  // Replace original buffer with summed result
  buffer.copyFrom(0, 0, accumulationBuffer, 0, 0, numSamples);
  if (buffer.getNumChannels() > 1)
    buffer.copyFrom(1, 0, accumulationBuffer, 1, 0, numSamples);
}

void MulticombFilter::reset() {
  for (auto &comb : combs) {
    comb->reset();
  }
}

void MulticombFilter::setParameter(int index, float value) {
  // Basic implementation: set global feedback or spread
  if (index == 0) { // Spread/Density
    for (int i = 0; i < (int)combs.size(); ++i) {
      float offset = (float)i * 5.0f * value;
      combs[i]->setDelay(10.0f + offset);
    }
  } else if (index == 1) { // Feedback
    for (auto &comb : combs) {
      comb->setFeedback(value * 0.99f);
    }
  }
}

void MulticombFilter::setBankParams(float baseDelayMs, float spread, float fb) {
  for (int i = 0; i < (int)combs.size(); ++i) {
    combs[i]->setDelay(baseDelayMs + (float)i * spread);
    combs[i]->setFeedback(fb);
  }
}

} // namespace dsp
} // namespace flowzone
