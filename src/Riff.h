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
  double sourceSampleRate{44100.0};

  Riff() : id(juce::Uuid()), captureTime(juce::Time::getCurrentTime()) {}

  // Move constructor/assignment
  Riff(Riff &&other) noexcept
      : id(other.id), name(std::move(other.name)),
        layerBuffers(std::move(other.layerBuffers)), bpm(other.bpm),
        bars(other.bars), captureTime(other.captureTime), layers(other.layers),
        sourceSampleRate(other.sourceSampleRate) {}

  Riff &operator=(Riff &&other) noexcept {
    id = other.id;
    name = std::move(other.name);
    layerBuffers = std::move(other.layerBuffers);
    bpm = other.bpm;
    bars = other.bars;
    captureTime = other.captureTime;
    layers = other.layers;
    sourceSampleRate = other.sourceSampleRate;
    return *this;
  }

  Riff(const Riff &) = default;
  Riff &operator=(const Riff &) = default;

  /** Merges new audio into this riff (layering). */
  void merge(const juce::AudioBuffer<float> &newAudio) {
    if (layers >= 8)
      return;

    juce::AudioBuffer<float> layerCopy;
    layerCopy.makeCopyOf(newAudio);
    layerBuffers.push_back(std::move(layerCopy));
    layers = static_cast<int>(layerBuffers.size());
  }

  /** Gets a summed version of all layers for playback. */
  void getCompositeAudio(juce::AudioBuffer<float> &output) const {
    if (layerBuffers.empty()) {
      output.setSize(0, 0);
      return;
    }

    output.makeCopyOf(layerBuffers[0]);
    for (size_t i = 1; i < layerBuffers.size(); ++i) {
      for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        if (ch < layerBuffers[i].getNumChannels()) {
          output.addFrom(ch, 0, layerBuffers[i], ch, 0, output.getNumSamples());
        }
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
    // Limit history size to 100 as per instructions
    if (history.size() > 100) {
      history.erase(history.begin());
    }
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
