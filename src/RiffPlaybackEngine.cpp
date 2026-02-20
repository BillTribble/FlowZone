#include "RiffPlaybackEngine.h"

void RiffPlaybackEngine::prepare(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;
  const juce::ScopedLock sl(lock);
  playingRiffs.clear();
}

void RiffPlaybackEngine::processNextBlock(
    juce::AudioBuffer<float> &outputBuffer, double targetBpm) {
  const juce::ScopedLock sl(lock);

  const int numSamples = outputBuffer.getNumSamples();
  const int outChannels = outputBuffer.getNumChannels();

  for (auto it = playingRiffs.begin(); it != playingRiffs.end();) {
    auto &riff = *it;

    const int riffSamples = riff->audio.getNumSamples();
    const int riffChannels = riff->audio.getNumChannels();
    const double speedRatio = targetBpm / riff->sourceBpm;

    for (int i = 0; i < numSamples; ++i) {
      const int posInt = static_cast<int>(riff->currentPosition);
      const int nextPosInt = (posInt + 1);
      const float fraction = static_cast<float>(riff->currentPosition - posInt);

      for (int ch = 0; ch < outChannels; ++ch) {
        if (ch < riffChannels) {
          float s1 = riff->audio.getSample(ch, posInt);
          float s2 =
              (nextPosInt < riffSamples)
                  ? riff->audio.getSample(ch, nextPosInt)
                  : (riff->looping ? riff->audio.getSample(ch, 0) : 0.0f);

          // Linear interpolation
          float interpolatedSample = s1 + fraction * (s2 - s1);
          outputBuffer.addSample(ch, i, interpolatedSample);
        }
      }

      riff->currentPosition += speedRatio;

      if (riff->currentPosition >= static_cast<double>(riffSamples)) {
        if (riff->looping) {
          riff->currentPosition = std::fmod(riff->currentPosition,
                                            static_cast<double>(riffSamples));
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
                                  double bpm, bool loop) {
  auto playing = std::make_unique<PlayingRiff>();
  playing->riffId = id;
  // Deep copy the audio
  playing->audio.makeCopyOf(audio);
  playing->currentPosition = 0.0;
  playing->sourceBpm = bpm;
  playing->looping = loop;
  playing->finished = false;

  const juce::ScopedLock sl(lock);
  playingRiffs.clear(); // Exclusive playback
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
