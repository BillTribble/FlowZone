#include "RetrospectiveBuffer.h"

namespace flowzone {

RetrospectiveBuffer::RetrospectiveBuffer() {}

RetrospectiveBuffer::~RetrospectiveBuffer() {}

void RetrospectiveBuffer::prepare(double sampleRate, int maxSeconds) {
  int numSamples = (int)(sampleRate * maxSeconds);
  // Support stereo for now
  buffer.setSize(2, numSamples);
  buffer.clear();
  writeIndex = 0;
  bufferSize = numSamples;
}

void RetrospectiveBuffer::pushBlock(const juce::AudioBuffer<float> &input) {
  if (bufferSize == 0)
    return;

  int numSamples = input.getNumSamples();
  int numChannels = std::min(input.getNumChannels(), buffer.getNumChannels());

  // Write input into circular buffer
  // Handle wrap-around
  int start1 = writeIndex;
  int block1 = std::min(numSamples, bufferSize - start1);
  int start2 = 0;
  int block2 = numSamples - block1;

  for (int ch = 0; ch < numChannels; ++ch) {
    buffer.copyFrom(ch, start1, input, ch, 0, block1);
    if (block2 > 0) {
      buffer.copyFrom(ch, start2, input, ch, block1, block2);
    }
  }

  writeIndex = (writeIndex + numSamples) % bufferSize;
}

void RetrospectiveBuffer::getPastAudio(int delayInSamples, int numSamples,
                                       juce::AudioBuffer<float> &destination) {
  if (bufferSize == 0)
    return;

  // delayInSamples is how far back from "now" (writeIndex)
  // read head = writeIndex - delayInSamples
  // Check bounds

  // We want to read 'numSamples' ending at 'writeIndex - delayInSamples'??
  // Usually retrospective means "give me the last 4 bars".
  // If the loop just ended, delayInSamples might be 0 (meaning up to current
  // moment).

  int readHead = writeIndex - delayInSamples - numSamples;
  // Wait, if delay is 0, we want samples ending at writeIndex.
  // So read start is writeIndex - numSamples.
  // If delay is N, read start is writeIndex - N - numSamples.

  // Handle negative wrap
  while (readHead < 0)
    readHead += bufferSize;
  readHead %= bufferSize;

  // We want to return however many channels we have stored
  int numChannels = buffer.getNumChannels();
  // Use setSize to ensure destination has enough storage.
  // keepExistingContent = false, clearExtraSpace = true, avoidReallocating =
  // false
  destination.setSize(numChannels, numSamples, false, true, false);

  int start1 = readHead;
  int block1 = std::min(numSamples, bufferSize - start1);
  int start2 = 0;
  int block2 = numSamples - block1;

  for (int ch = 0; ch < numChannels; ++ch) {
    destination.copyFrom(ch, 0, buffer, ch, start1, block1);
    if (block2 > 0) {
      destination.copyFrom(ch, block1, buffer, ch, start2, block2);
    }
  }
}

} // namespace flowzone
