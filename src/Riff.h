#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

/**
 * Represents a single captured loop (a "riff").
 * Holds the actual audio data and metadata.
 */
struct Riff {
  juce::Uuid id;
  juce::String name;
  std::vector<juce::AudioBuffer<float>> layerBuffers;
  double bpm{120.0};
  int bars{1};
  juce::Time captureTime;
  int layers{1};
  std::vector<int> layerBars;
  double sourceSampleRate{44100.0};
  juce::String source{"Microphone"};

  Riff() : id(juce::Uuid()), captureTime(juce::Time::getCurrentTime()) {}

  // Move constructor/assignment
  Riff(Riff &&other) noexcept
      : id(other.id), name(std::move(other.name)),
        layerBuffers(std::move(other.layerBuffers)), bpm(other.bpm),
        bars(other.bars), captureTime(other.captureTime), layers(other.layers),
        layerBars(std::move(other.layerBars)),
        sourceSampleRate(other.sourceSampleRate),
        source(std::move(other.source)) {}

  Riff &operator=(Riff &&other) noexcept {
    id = other.id;
    name = std::move(other.name);
    layerBuffers = std::move(other.layerBuffers);
    bpm = other.bpm;
    bars = other.bars;
    captureTime = other.captureTime;
    layers = other.layers;
    layerBars = std::move(other.layerBars);
    sourceSampleRate = other.sourceSampleRate;
    source = std::move(other.source);
    return *this;
  }

  Riff(const Riff &) = default;
  Riff &operator=(const Riff &) = default;

  /**
   * Sums all current layers into a single composite buffer and replaces
   * the layer list with this summed result.
   */
  void sumToSingleLayer() {
    if (layerBuffers.empty())
      return;

    juce::AudioBuffer<float> composite;
    getCompositeAudio(composite);

    layerBuffers.clear();
    layerBuffers.push_back(std::move(composite));
    layers = 1;
  }

  /** Merges new audio into this riff (layering). */
  void merge(const juce::AudioBuffer<float> &newAudio, int barsToMerge) {
    if (layers >= 8)
      return;

    if (barsToMerge > bars)
      bars = barsToMerge;

    juce::AudioBuffer<float> layerCopy;
    layerCopy.makeCopyOf(newAudio);
    layerBuffers.push_back(std::move(layerCopy));
    layerBars.push_back(barsToMerge);
    layers = static_cast<int>(layerBuffers.size());
  }

  /** Gets a summed version of all layers for playback. */
  void getCompositeAudio(juce::AudioBuffer<float> &output) const {
    if (layerBuffers.empty()) {
      output.setSize(0, 0);
      return;
    }

    // Determine the max length in samples based on the current 'bars'
    int maxSamples = 0;
    for (const auto &buf : layerBuffers) {
      if (buf.getNumSamples() > maxSamples)
        maxSamples = buf.getNumSamples();
    }

    // Ensure output is large enough for the master length
    // (Note: bars might have been updated by merge)
    output.setSize(2, maxSamples);
    output.clear();

    for (size_t i = 0; i < layerBuffers.size(); ++i) {
      const auto &buf = layerBuffers[i];
      int layerSamples = buf.getNumSamples();

      if (layerSamples == 0)
        continue;

      // Loop shorter layers to fill the master duration
      int samplesCopied = 0;
      while (samplesCopied < maxSamples) {
        int samplesToCopy = std::min(layerSamples, maxSamples - samplesCopied);
        for (int ch = 0; ch < output.getNumChannels(); ++ch) {
          if (ch < buf.getNumChannels()) {
            output.addFrom(ch, samplesCopied, buf, ch, 0, samplesToCopy);
          }
        }
        samplesCopied += samplesToCopy;
      }
    }
  }
};

/**
 * Manages a history of Riffs.
 */
class RiffHistory {
public:
  const Riff &addRiff(Riff &&riff) {
    history.push_back(std::move(riff));
    updateCounter++;
    return history.back();
  }

  Riff *getLastRiff() { return history.empty() ? nullptr : &history.back(); }

  void signalUpdate() { updateCounter++; }

  const std::vector<Riff> &getHistory() const { return history; }
  std::vector<Riff> &getHistoryRW() { return history; }
  size_t size() const { return history.size(); }
  int getUpdateCounter() const { return updateCounter; }

private:
  std::vector<Riff> history;
  int updateCounter{0};
};
