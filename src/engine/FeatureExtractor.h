#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

namespace flowzone {

/**
 * @brief Extracts waveform features from audio for visualization
 * 
 * Uses double-buffered atomic exchange for lock-free access
 * Target: 30fps update rate
 */
class FeatureExtractor {
public:
  FeatureExtractor();
  ~FeatureExtractor();

  void prepare(double sampleRate, int samplesPerBlock);
  
  // Called from audio thread to push waveform data
  void pushAudioBlock(const juce::AudioBuffer<float> &block);
  
  // Called from UI/message thread to get latest waveform
  std::vector<float> getLatestWaveform();
  
  // Get downsampled waveform (256 points for UI display)
  std::vector<float> getDownsampledWaveform(int targetSize = 256);

private:
  static constexpr int kBufferSeconds = 97; // Match retro buffer size
  static constexpr int kDownsamplePoints = 256;
  
  juce::AudioBuffer<float> ringBuffer;
  int writePos = 0;
  int ringBufferSize = 0;
  double currentSampleRate = 44100.0;
  
  // Double buffer for lock-free exchange
  std::vector<float> bufferA;
  std::vector<float> bufferB;
  std::atomic<std::vector<float>*> currentBuffer{&bufferA};
  std::atomic<std::vector<float>*> displayBuffer{&bufferB};
  
  juce::CriticalSection lock;
  
  void downsampleToBuffer(std::vector<float> &dest, int targetSize);
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeatureExtractor)
};

} // namespace flowzone
