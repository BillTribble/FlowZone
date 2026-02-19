#pragma once
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

//==============================================================================
/// A lock-free circular audio buffer designed for the retrospective capture
/// pipeline. Audio thread writes via pushBlock(); message thread reads via
/// getWaveformData(). Single writer / single reader — no mutex required.
class RetrospectiveBuffer {
public:
  RetrospectiveBuffer() = default;
  ~RetrospectiveBuffer() = default;

  //==========================================================================
  /// Called from prepareToPlay() (message thread) before audio starts.
  /// Pre-allocates maxSeconds * sampleRate frames of stereo storage.
  void prepare(double sampleRate, int maxSeconds);

  //==========================================================================
  /// Called from getNextAudioBlock() (audio thread). Writes numSamples from
  /// each channel into the ring buffer. Never allocates or locks.
  void pushBlock(const float *const *channelData, int numChannels,
                 int numSamples) noexcept;

  //==========================================================================
  /// Called from the message/timer thread. Returns a vector of numOutputPoints
  /// peak-absolute values downsampled from the most recent numRecentSamples
  /// audio frames.  Index 0 = oldest, last index = newest (right side of
  /// panel).
  [[nodiscard]] std::vector<float> getWaveformData(int numRecentSamples,
                                                   int numOutputPoints) const;

  //==========================================================================
  /// Deep-copies the most recent numSamples frames from the ring buffer into
  /// dest. The destination buffer is resized to (ringBuffer channels,
  /// numSamples).
  void getAudioRegion(juce::AudioBuffer<float> &dest, int numSamples) const;

  /// Total capacity of the ring buffer in frames.
  [[nodiscard]] int capacityFrames() const noexcept { return bufferCapacity; }

private:
  juce::AudioBuffer<float> ringBuffer; // pre-allocated, stereo
  int bufferCapacity{0};               // total frames in buffer
  std::atomic<int> writeIndex{0};      // next write position (audio thread)

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetrospectiveBuffer)
};
