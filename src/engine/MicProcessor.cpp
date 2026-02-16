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

  // Update peak level for metering
  updatePeakLevel(internalBuffer);

  // ALWAYS write processed audio to output buffer for retrospective capture
  // This is critical - the retrospective buffer needs the processed audio
  // regardless of whether monitoring is enabled
  for (int ch = 0; ch < outputBuffer.getNumChannels() && ch < numChannels; ++ch) {
    outputBuffer.copyFrom(ch, 0, internalBuffer, ch, 0, numSamples);
  }
  
  // If monitoring is disabled, we still wrote to outputBuffer for retro capture,
  // but we won't add it to the main mix in FlowEngine (handled elsewhere)
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

void MicProcessor::updatePeakLevel(const juce::AudioBuffer<float> &buffer) {
  float peak = 0.0f;
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
    peak = std::max(peak, channelPeak);
  }
  
  // Simple peak decay
  float currentPeak = peakLevel.load();
  if (peak > currentPeak) {
    peakLevel.store(peak);
  } else {
    // Decay slowly
    peakLevel.store(currentPeak * 0.95f);
  }
}

} // namespace engine
} // namespace flowzone
