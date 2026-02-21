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
  void processNextBlock(juce::AudioBuffer<float> &dryBuffer,
                        juce::AudioBuffer<float> &wetBuffer, double targetBpm,
                        int numSamplesToProcess, uint8_t layerMask = 0xFF);

  /**
   * Starts playback of a riff.
   */
  void playRiff(const Riff &riff, bool loop = false);

  bool isRiffPlaying(const juce::Uuid &id) const;
  juce::Uuid getCurrentlyPlayingRiffId() const;

private:
  struct PlayingRiff {
    juce::Uuid riffId;
    std::vector<juce::AudioBuffer<float>> layers;
    std::vector<float> layerGains;
    double currentPosition{0.0};
    double sourceBpm{120.0};
    double sourceSampleRate{44100.0};
    bool looping{false};
    bool finished{false};
    int totalBars{1};
  };

  std::vector<std::unique_ptr<PlayingRiff>> playingRiffs;
  juce::CriticalSection lock;

  double currentSampleRate{44100.0};
};
