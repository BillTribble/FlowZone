#include "MicProcessor.h"
#include <cmath>

namespace flowzone {
namespace engine {

MicProcessor::MicProcessor() {
  reverbParams.roomSize = 0.5f;
  reverbParams.damping = 0.5f;
  reverbParams.wetLevel = 0.33f;
  reverbParams.dryLevel = 1.0f;
  reverbParams.width = 1.0f;
  reverb.setParameters(reverbParams);
}

void MicProcessor::prepare(double sampleRate, int samplesPerBlock) {
  reverb.setSampleRate(sampleRate);
  internalBuffer.setSize(2, samplesPerBlock);
}

void MicProcessor::process(const juce::AudioBuffer<float> &inputBuffer,
                           juce::AudioBuffer<float> &outputBuffer) {
  int numSamples = inputBuffer.getNumSamples();
  int numChannels = inputBuffer.getNumChannels();

  // Copy input to internal buffer for processing
  internalBuffer.setSize(numChannels, numSamples, false, false, true);
  for (int ch = 0; ch < numChannels; ++ch) {
    internalBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);
  }

  // Apply Input Gain
  applyGain(internalBuffer);

  // Apply Reverb (before retrospective capture - as per Spec)
  if (reverbLevel > 0.0f) {
    if (numChannels == 1) {
      reverb.processMono(internalBuffer.getWritePointer(0), numSamples);
    } else {
      reverb.processStereo(internalBuffer.getWritePointer(0),
                           internalBuffer.getWritePointer(1), numSamples);
    }
  }

  // Direct Monitoring
  if (monitorEnabled) {
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, 0, internalBuffer, ch % numChannels, 0,
                           numSamples);
    }
  }
}

void MicProcessor::reset() { reverb.reset(); }

void MicProcessor::setInputGain(float gainDb) {
  inputGain = std::pow(10.0f, gainDb / 20.0f);
}

void MicProcessor::setMonitorEnabled(bool enabled) { monitorEnabled = enabled; }

void MicProcessor::setReverbLevel(float level) {
  reverbLevel = level;
  reverbParams.wetLevel = level;
  reverb.setParameters(reverbParams);
}

void MicProcessor::setMonitorUntilLooped(bool enabled) {
  monitorUntilLooped = enabled;
}

void MicProcessor::applyGain(juce::AudioBuffer<float> &buffer) {
  if (std::abs(inputGain - 1.0f) > 0.001f) {
    buffer.applyGain(inputGain);
  }
}

} // namespace engine
} // namespace flowzone
