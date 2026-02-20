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
  void processNextBlock(juce::AudioBuffer<float> &outputBuffer,
                        double targetBpm, int numSamplesToProcess);

  /**
   * Starts playback of a riff.
   */
  void playRiff(const juce::Uuid &id, const juce::AudioBuffer<float> &audio,
                double sourceBpm, bool loop = false);

  bool isRiffPlaying(const juce::Uuid &id) const;

private:
  struct PlayingRiff {
    juce::Uuid riffId;
    juce::AudioBuffer<float> audio;
    double currentPosition{0.0};
    double sourceBpm{120.0};
    bool looping{false};
    bool finished{false};
  };

  std::vector<std::unique_ptr<PlayingRiff>> playingRiffs;
  juce::CriticalSection lock;

  double currentSampleRate{44100.0};
};
