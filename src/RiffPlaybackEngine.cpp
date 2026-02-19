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
    const int remaining = riffSamples - riff->currentPosition;
    const int toWrite = std::min(numSamples, remaining);

    if (toWrite > 0) {
      for (int ch = 0; ch < outChannels; ++ch) {
        if (ch < riffChannels) {
          outputBuffer.addFrom(ch, 0, riff->audio, ch, riff->currentPosition,
                               toWrite);
        }
      }
      riff->currentPosition += toWrite;
    }

    if (riff->currentPosition >= riffSamples) {
      it = playingRiffs.erase(it);
    } else {
      ++it;
    }
  }
}

void RiffPlaybackEngine::playRiff(const Riff &riff) {
  auto playing = std::make_unique<PlayingRiff>();
  // Deep copy the audio to avoid lifetime issues in this prototype
  playing->audio.makeCopyOf(riff.audio);
  playing->currentPosition = 0;
  playing->finished = false;

  const juce::ScopedLock sl(lock);
  playingRiffs.push_back(std::move(playing));
}
