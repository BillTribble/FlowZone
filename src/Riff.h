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
  juce::AudioBuffer<float> audio;
  double bpm{120.0};
  int bars{1};
  juce::Time captureTime;
  int layers{1};

  Riff() : id(juce::Uuid()), captureTime(juce::Time::getCurrentTime()) {}

  // Move constructor/assignment for efficiency with AudioBuffer
  Riff(Riff &&other) noexcept
      : id(other.id), name(std::move(other.name)),
        audio(std::move(other.audio)), bpm(other.bpm), bars(other.bars),
        captureTime(other.captureTime), layers(other.layers) {}

  Riff &operator=(Riff &&other) noexcept {
    id = other.id;
    name = std::move(other.name);
    audio = std::move(other.audio);
    bpm = other.bpm;
    bars = other.bars;
    captureTime = other.captureTime;
    layers = other.layers;
    return *this;
  }

  // Prevent accidental copies of large audio buffers if possible,
  // but we might need deep copy for the initial capture.
  Riff(const Riff &) = default;
  Riff &operator=(const Riff &) = default;

  /** Merges new audio into this riff (layering). */
  void merge(const juce::AudioBuffer<float> &newAudio) {
    if (layers >= 8)
      return;

    const int numSamples = audio.getNumSamples();
    if (numSamples == 0) {
      audio.makeCopyOf(newAudio);
    } else {
      const int samplesToCopy = std::min(numSamples, newAudio.getNumSamples());
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        if (ch < newAudio.getNumChannels()) {
          audio.addFrom(ch, 0, newAudio, ch, 0, samplesToCopy);
        }
      }
    }
    layers++;
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
  size_t size() const { return history.size(); }
  int getUpdateCounter() const { return updateCounter; }

private:
  std::vector<Riff> history;
  int updateCounter{0};
};
