#include "RetrospectiveBuffer.h"
#include <algorithm>
#include <cmath>

//==============================================================================
void RetrospectiveBuffer::prepare(double sampleRate, int maxSeconds) {
  // Called on message thread before audio starts — allocation is safe here.
  bufferCapacity = static_cast<int>(sampleRate * maxSeconds);
  ringBuffer.setSize(2, bufferCapacity, /*keepExistingContent=*/false,
                     /*clearExtraSpace=*/true, /*avoidReallocating=*/false);
  ringBuffer.clear();
  writeIndex.store(0);
}

//==============================================================================
void RetrospectiveBuffer::pushBlock(const float *const *channelData,
                                    int numChannels, int numSamples) noexcept {
  if (bufferCapacity == 0 || channelData == nullptr || numSamples <= 0)
    return;

  const int numBufChannels = ringBuffer.getNumChannels();
  int wi = writeIndex.load(std::memory_order_relaxed);

  for (int i = 0; i < numSamples; ++i) {
    int pos = (wi + i) % bufferCapacity;
    for (int ch = 0; ch < numBufChannels; ++ch) {
      float sample = 0.0f;
      if (ch < numChannels && channelData[ch] != nullptr)
        sample = channelData[ch][i];
      ringBuffer.setSample(ch, pos, sample);
    }
  }

  // Advance write index atomically so the reader always sees a consistent
  // value.
  wi = (wi + numSamples) % bufferCapacity;
  writeIndex.store(wi, std::memory_order_release);
}

//==============================================================================
std::vector<float>
RetrospectiveBuffer::getWaveformData(int numRecentSamples, int numOutputPoints,
                                     int offsetSamples) const {
  std::vector<float> result(static_cast<size_t>(numOutputPoints), 0.0f);

  if (bufferCapacity == 0 || numRecentSamples <= 0 || numOutputPoints <= 0)
    return result;

  // Clamp to available capacity
  numRecentSamples = std::min(numRecentSamples, bufferCapacity);
  offsetSamples = std::max(0, offsetSamples);

  // Snapshot the write head
  const int wi = writeIndex.load(std::memory_order_acquire);
  const int numBufChannels = ringBuffer.getNumChannels();

  // Downsample: divide numRecentSamples into numOutputPoints buckets.
  const double samplesPerPoint = static_cast<double>(numRecentSamples) /
                                 static_cast<double>(numOutputPoints);

  for (int pt = 0; pt < numOutputPoints; ++pt) {
    int startOffset = static_cast<int>(pt * samplesPerPoint);
    int endOffset = static_cast<int>((pt + 1) * samplesPerPoint);
    endOffset = std::min(endOffset, numRecentSamples);

    float peak = 0.0f;
    for (int off = startOffset; off < endOffset; ++off) {
      // Index into the ring buffer, walking backwards from (wi - offsetSamples)
      int ringPos =
          (wi - offsetSamples - numRecentSamples + off + bufferCapacity) %
          bufferCapacity;
      for (int ch = 0; ch < numBufChannels; ++ch) {
        float s = std::abs(ringBuffer.getSample(ch, ringPos));
        if (s > peak)
          peak = s;
      }
    }
    result[static_cast<size_t>(pt)] = peak;
  }

  return result;
}

//==============================================================================
void RetrospectiveBuffer::getAudioRegion(juce::AudioBuffer<float> &dest,
                                         int numSamples,
                                         int offsetSamples) const {
  if (bufferCapacity == 0 || numSamples <= 0) {
    dest.setSize(ringBuffer.getNumChannels(), 0);
    return;
  }

  // Clamp to available capacity
  numSamples = std::min(numSamples, bufferCapacity);
  offsetSamples = std::max(0, offsetSamples);

  const int numBufChannels = ringBuffer.getNumChannels();
  dest.setSize(numBufChannels, numSamples, false, true, true);

  // Snapshot the write head
  const int wi = writeIndex.load(std::memory_order_acquire);

  for (int ch = 0; ch < numBufChannels; ++ch) {
    // Determine the end position (wi - offsetSamples)
    int endPos = (wi - offsetSamples + bufferCapacity) % bufferCapacity;
    // Determine the start position (endPos - numSamples)
    int startPos = (endPos - numSamples + bufferCapacity) % bufferCapacity;

    if (startPos + numSamples <= bufferCapacity) {
      // Linear copy in one go
      dest.copyFrom(ch, 0, ringBuffer, ch, startPos, numSamples);
    } else {
      // Wrapped copy: two parts
      int firstPartSize = bufferCapacity - startPos;
      dest.copyFrom(ch, 0, ringBuffer, ch, startPos, firstPartSize);
      dest.copyFrom(ch, firstPartSize, ringBuffer, ch, 0,
                    numSamples - firstPartSize);
    }
  }
}
