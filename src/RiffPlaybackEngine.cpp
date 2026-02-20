#include "RiffPlaybackEngine.h"

void RiffPlaybackEngine::prepare(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;
  const juce::ScopedLock sl(lock);
  playingRiffs.clear();
}

void RiffPlaybackEngine::processNextBlock(
    juce::AudioBuffer<float> &outputBuffer, double targetBpm,
    int numSamplesToProcess) {
  const juce::ScopedLock sl(lock);

  const int outChannels = outputBuffer.getNumChannels();

  for (auto it = playingRiffs.begin(); it != playingRiffs.end();) {
    auto &riff = *it;
    const int riffSamples = riff->audio.getNumSamples();
    const int riffChannels = riff->audio.getNumChannels();
    // Correct speed for both BPM and Sample Rate differences
    const double speedRatio = (targetBpm / riff->sourceBpm) *
                              (riff->sourceSampleRate / currentSampleRate);

    for (int i = 0; i < numSamplesToProcess; ++i) {
      const int posInt = static_cast<int>(riff->currentPosition);
      const int nextPosInt = (posInt + 1);
      const float fraction = static_cast<float>(riff->currentPosition - posInt);

      for (int outCh = 0; outCh < outChannels; ++outCh) {
        // If riff is mono, add to all output channels. If stereo, match 1:1.
        int inCh = (riffChannels == 1) ? 0 : outCh;
        if (inCh < riffChannels) {
          const auto *riffData = riff->audio.getReadPointer(inCh);
          float s0 = riffData[posInt];
          float s1 = (nextPosInt < riffSamples)
                         ? riffData[nextPosInt]
                         : (riff->looping ? riffData[0] : 0.0f);
          float interp = s0 + (s1 - s0) * fraction;

          outputBuffer.addFrom(outCh, i, &interp, 1);
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
                                  double sourceBpm, double sourceSampleRate,
                                  bool loop) {
  const juce::ScopedLock sl(lock);

  auto playingRiff = std::make_unique<PlayingRiff>();
  playingRiff->riffId = id;
  // Deep copy the audio
  playingRiff->audio.makeCopyOf(audio);
  playingRiff->currentPosition = 0.0;
  playingRiff->sourceBpm = sourceBpm;
  playingRiff->sourceSampleRate = sourceSampleRate;
  playingRiff->looping = loop;
  playingRiff->finished = false;

  playingRiffs.clear(); // Exclusive playback
  playingRiffs.push_back(std::move(playingRiff));
}

bool RiffPlaybackEngine::isRiffPlaying(const juce::Uuid &id) const {
  const juce::ScopedLock sl(lock);
  for (const auto &pr : playingRiffs) {
    if (pr->riffId == id)
      return true;
  }
  return false;
}
juce::Uuid RiffPlaybackEngine::getCurrentlyPlayingRiffId() const {
  const juce::ScopedLock sl(lock);
  if (!playingRiffs.empty())
    return playingRiffs.front()->riffId;
  return juce::Uuid();
}
