#include "RiffPlaybackEngine.h"

void RiffPlaybackEngine::prepare(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;
  const juce::ScopedLock sl(lock);
  playingRiffs.clear();
}

void RiffPlaybackEngine::processNextBlock(
    juce::AudioBuffer<float> &outputBuffer) {
  const juce::ScopedLock sl(lock);

  const int numSamples = outputBuffer.getNumSamples();
  const int outChannels = outputBuffer.getNumChannels();

  for (auto it = playingRiffs.begin(); it != playingRiffs.end();) {
    auto &riff = *it;

    const int riffSamples = riff->audio.getNumSamples();
    const int riffChannels = riff->audio.getNumChannels();

    int samplesToProcess = numSamples;
    int outOffset = 0;

    while (samplesToProcess > 0) {
      const int remainingInRiff = riffSamples - riff->currentPosition;
      const int toProcessNow = std::min(samplesToProcess, remainingInRiff);

      if (toProcessNow > 0) {
        for (int ch = 0; ch < outChannels; ++ch) {
          if (ch < riffChannels) {
            outputBuffer.addFrom(ch, outOffset, riff->audio, ch,
                                 riff->currentPosition, toProcessNow);
          }
        }
        riff->currentPosition += toProcessNow;
        outOffset += toProcessNow;
        samplesToProcess -= toProcessNow;
      }

      if (riff->currentPosition >= riffSamples) {
        if (riff->looping) {
          riff->currentPosition = 0;
        } else {
          riff->finished = true;
          break;
        }
      }
    }

    if (riff->finished) {
      it = playingRiffs.erase(it);
    } else {
      ++it;
    }
  }
}

void RiffPlaybackEngine::playRiff(const juce::Uuid &id,
                                  const juce::AudioBuffer<float> &audio,
                                  bool loop) {
  auto playing = std::make_unique<PlayingRiff>();
  playing->riffId = id;
  // Deep copy the audio to avoid lifetime issues in this prototype
  playing->audio.makeCopyOf(audio);
  playing->currentPosition = 0;
  playing->looping = loop;
  playing->finished = false;

  const juce::ScopedLock sl(lock);
  playingRiffs.clear(); // Exclusive playback: stop any currently playing riffs
  playingRiffs.push_back(std::move(playing));
}

bool RiffPlaybackEngine::isRiffPlaying(const juce::Uuid &id) const {
  const juce::ScopedLock sl(lock);
  for (const auto &pr : playingRiffs) {
    if (pr->riffId == id)
      return true;
  }
  return false;
}
