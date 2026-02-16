#include "FeatureExtractor.h"
#include <algorithm>
#include <cmath>

namespace flowzone {

FeatureExtractor::FeatureExtractor() {
  bufferA.resize(kDownsamplePoints, 0.0f);
  bufferB.resize(kDownsamplePoints, 0.0f);
}

FeatureExtractor::~FeatureExtractor() {}

void FeatureExtractor::prepare(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  ringBufferSize = (int)(sampleRate * kBufferSeconds);
  ringBuffer.setSize(2, ringBufferSize);
  ringBuffer.clear();
  writePos = 0;
  
  juce::ignoreUnused(samplesPerBlock);
}

void FeatureExtractor::pushAudioBlock(const juce::AudioBuffer<float> &block) {
  int numSamples = block.getNumSamples();
  int numChannels = std::min(block.getNumChannels(), ringBuffer.getNumChannels());
  
  // Write to ring buffer
  for (int ch = 0; ch < numChannels; ++ch) {
    for (int i = 0; i < numSamples; ++i) {
      ringBuffer.setSample(ch, writePos, block.getSample(ch, i));
      writePos = (writePos + 1) % ringBufferSize;
    }
  }
}

std::vector<float> FeatureExtractor::getLatestWaveform() {
  std::vector<float> result;
  result.reserve(ringBufferSize);
  
  // Read ring buffer in chronological order
  for (int i = 0; i < ringBufferSize; ++i) {
    int readPos = (writePos + i) % ringBufferSize;
    // Mono mix
    float sample = 0.0f;
    for (int ch = 0; ch < ringBuffer.getNumChannels(); ++ch) {
      sample += ringBuffer.getSample(ch, readPos);
    }
    sample /= (float)ringBuffer.getNumChannels();
    result.push_back(sample);
  }
  
  return result;
}

std::vector<float> FeatureExtractor::getDownsampledWaveform(int targetSize) {
  std::vector<float> result(targetSize, 0.0f);
  
  if (ringBufferSize == 0 || targetSize == 0)
    return result;
  
  // Downsample by taking max absolute value in each window
  float samplesPerPoint = (float)ringBufferSize / (float)targetSize;
  
  for (int i = 0; i < targetSize; ++i) {
    int startSample = (int)(i * samplesPerPoint);
    int endSample = (int)((i + 1) * samplesPerPoint);
    endSample = std::min(endSample, ringBufferSize);
    
    float maxVal = 0.0f;
    for (int s = startSample; s < endSample; ++s) {
      int readPos = (writePos + s) % ringBufferSize;
      
      // Mono mix and get absolute value
      float sampleVal = 0.0f;
      for (int ch = 0; ch < ringBuffer.getNumChannels(); ++ch) {
        sampleVal += std::abs(ringBuffer.getSample(ch, readPos));
      }
      sampleVal /= (float)ringBuffer.getNumChannels();
      
      maxVal = std::max(maxVal, sampleVal);
    }
    
    result[i] = maxVal;
  }
  
  return result;
}

void FeatureExtractor::downsampleToBuffer(std::vector<float> &dest, int targetSize) {
  dest = getDownsampledWaveform(targetSize);
}

} // namespace flowzone
