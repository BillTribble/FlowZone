#pragma once
#include "Riff.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

/**
 * A simple playback engine for riffs.
 * Supports multiple concurrent one-shot playbacks.
 */
class RiffPlaybackEngine {
public:
  RiffPlaybackEngine() = default;
  ~RiffPlaybackEngine() = default;

  void prepare(double sampleRate, int samplesPerBlock);
  void processNextBlock(juce::AudioBuffer<float> &outputBuffer);

  /**
   * Starts playback of a riff.
   * This makes a copy of the riff's audio for playback or ideally just holds
   * a reference if we can guarantee lifetime, but for a prototype, let's keep
   * it safe.
   */
  void playRiff(const Riff &riff);

private:
  struct PlayingRiff {
    juce::AudioBuffer<float> audio;
    int currentPosition{0};
    bool finished{false};
  };

  std::vector<std::unique_ptr<PlayingRiff>> playingRiffs;
  juce::CriticalSection lock;

  double currentSampleRate{44100.0};
};
