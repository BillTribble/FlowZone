#include "RiffPlaybackEngine.h"

void RiffPlaybackEngine::prepare(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;
  const juce::ScopedLock sl(lock);
  playingRiffs.clear();
}

void RiffPlaybackEngine::processNextBlock(juce::AudioBuffer<float> &dryBuffer,
                                          juce::AudioBuffer<float> &wetBuffer,
                                          double targetBpm,
                                          int numSamplesToProcess,
                                          double masterPpq, uint8_t layerMask) {
  const juce::ScopedLock sl(lock);

  const int dryChannels = dryBuffer.getNumChannels();
  const int wetChannels = wetBuffer.getNumChannels();

  for (auto it = playingRiffs.begin(); it != playingRiffs.end();) {
    auto &riff = *it;

    const int numLayers = static_cast<int>(riff->layers.size());
    if (numLayers == 0) {
      it = playingRiffs.erase(it);
      continue;
    }

    const int riffSamples = riff->layers[0].getNumSamples();
    const double riffBeats = riff->totalBars * 4.0;

    // Beats per sample at the Riff's native SampleRate/BPM
    const double nativeSecondsPerBeat = 60.0 / riff->sourceBpm;
    const double nativeSamplesPerBeat =
        nativeSecondsPerBeat * riff->sourceSampleRate;

    // Current increment per hardware sample
    const double beatsPerSecond = targetBpm / 60.0;
    const double beatsPerSample = beatsPerSecond / currentSampleRate;

    for (int i = 0; i < numSamplesToProcess; ++i) {
      // Phase-locking: Calculate exact position based on global PPQ
      // masterPpq + (i * beatsPerSample) is the exact beat for this sample
      const double currentPpq = masterPpq + (i * beatsPerSample);

      // Wrap the PPQ to the riff's length
      const double wrappedPpq = std::fmod(currentPpq, riffBeats);

      // Position in samples (slaved to global clock)
      const double posSamples = wrappedPpq * nativeSamplesPerBeat;

      const int posInt = static_cast<int>(posSamples);
      const int nextPosInt = (posInt + 1) % riffSamples;
      const float fraction = static_cast<float>(posSamples - posInt);

      for (int layerIdx = 0; layerIdx < numLayers; ++layerIdx) {
        bool isWet = (layerMask & (1 << layerIdx));
        auto &targetBuffer = isWet ? wetBuffer : dryBuffer;
        const int outChannels = targetBuffer.getNumChannels();

        const auto &layerBuf = riff->layers[layerIdx];
        const int layerChannels = layerBuf.getNumChannels();
        const float gain = riff->layerGains[layerIdx];

        for (int outCh = 0; outCh < outChannels; ++outCh) {
          int inCh = (layerChannels == 1) ? 0 : outCh;
          if (inCh < layerChannels) {
            const auto *layerData = layerBuf.getReadPointer(inCh);
            float s0 = layerData[posInt];
            float s1 = layerData[nextPosInt];
            float interp = (s0 + (s1 - s0) * fraction) * gain;

            targetBuffer.addFrom(outCh, i, &interp, 1);
          }
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

void RiffPlaybackEngine::playRiff(const Riff &riff, bool loop) {
  const juce::ScopedLock sl(lock);

  double inheritedPosition = 0.0;
  if (!playingRiffs.empty()) {
    inheritedPosition = playingRiffs.front()->currentPosition;
    playingRiffs.clear(); // Ensure exclusive playback and correct inheritance
  }

  auto playingRiff = std::make_unique<PlayingRiff>();
  playingRiff->riffId = riff.id;

  // Copy all layers
  for (const auto &buf : riff.layerBuffers) {
    juce::AudioBuffer<float> layerCopy;
    layerCopy.makeCopyOf(buf);
    playingRiff->layers.push_back(std::move(layerCopy));
  }
  playingRiff->layerGains = riff.layerGains;

  // Inherit position and wrap if necessary
  if (!playingRiff->layers.empty() &&
      playingRiff->layers[0].getNumSamples() > 0) {
    int firstLayerSamples = playingRiff->layers[0].getNumSamples();
    playingRiff->currentPosition =
        std::fmod(inheritedPosition, (double)firstLayerSamples);
  } else {
    playingRiff->currentPosition = 0.0;
  }

  playingRiff->sourceBpm = riff.bpm;
  playingRiff->sourceSampleRate = riff.sourceSampleRate;
  playingRiff->looping = loop;
  playingRiff->finished = false;
  playingRiff->totalBars = riff.bars;

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
