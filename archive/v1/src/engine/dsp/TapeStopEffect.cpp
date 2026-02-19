#include "TapeStopEffect.h"
#include <algorithm>

namespace flowzone {
namespace dsp {

TapeStopEffect::TapeStopEffect() {
  historyBufferL.resize(88200, 0.0f); // 2s at 44.1k
  historyBufferR.resize(88200, 0.0f);
}

void TapeStopEffect::prepare(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  historyBufferL.resize((int)(sampleRate * 2.0), 0.0f);
  historyBufferR.resize((int)(sampleRate * 2.0), 0.0f);
  reset();
}

void TapeStopEffect::process(juce::AudioBuffer<float> &buffer) {
  int numSamples = buffer.getNumSamples();
  auto *left = buffer.getWritePointer(0);
  auto *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  int bufferSize = (int)historyBufferL.size();

  for (int i = 0; i < numSamples; ++i) {
    float inL = left[i];
    float inR = right ? right[i] : inL;

    // Continuous capture
    historyBufferL[writeIndex] = inL;
    historyBufferR[writeIndex] = inR;

    if (isStopping || currentSpeed < 0.99f) {
      // Speed evolution
      if (isStopping)
        currentSpeed = std::max(0.0f, currentSpeed - stopRate);
      else
        currentSpeed = std::min(1.0f, currentSpeed + stopRate * 2.0f);

      // Resample from history
      int idx1 = (int)readPhase % bufferSize;
      int idx2 = (idx1 + 1) % bufferSize;
      float frac = readPhase - std::floor(readPhase);

      left[i] =
          historyBufferL[idx1] * (1.0f - frac) + historyBufferL[idx2] * frac;
      if (right)
        right[i] =
            historyBufferR[idx1] * (1.0f - frac) + historyBufferR[idx2] * frac;

      readPhase += currentSpeed;
      if (readPhase >= (float)bufferSize)
        readPhase -= (float)bufferSize;
    } else {
      readPhase = (float)writeIndex;
    }

    writeIndex = (writeIndex + 1) % bufferSize;
  }
}

void TapeStopEffect::reset() {
  std::fill(historyBufferL.begin(), historyBufferL.end(), 0.0f);
  std::fill(historyBufferR.begin(), historyBufferR.end(), 0.0f);
  writeIndex = 0;
  readPhase = 0.0f;
  currentSpeed = 1.0f;
}

void TapeStopEffect::setParameter(int index, float value) {
  if (index == 0)
    setStop(value > 0.5f);
  else if (index == 1)
    stopRate = 0.0001f + value * 0.01f;
}

void TapeStopEffect::setStop(bool s) { isStopping = s; }

} // namespace dsp
} // namespace flowzone
