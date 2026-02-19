#pragma once

#include <JuceHeader.h>
#include <vector>

namespace flowzone {

// bd-1u3: Retrospective Capture Buffer
class RetrospectiveBuffer {
public:
  RetrospectiveBuffer();
  ~RetrospectiveBuffer();

  void prepare(double sampleRate, int maxSeconds);

  // Push incoming audio block (multi-channel)
  void pushBlock(const juce::AudioBuffer<float> &input);

  // Retrieve past audio
  // delayInSamples: how far back to start reading (0 = now, >0 = past)
  // numSamples: how many samples to read
  // destination: buffer to write to
  void getPastAudio(int delayInSamples, int numSamples,
                    juce::AudioBuffer<float> &destination);

  // Get downsampled waveform data for UI visualization
  // Returns mono-summed, downsampled peak data
  // targetSamples: desired number of output samples (e.g. 256 for UI)
  std::vector<float> getWaveformData(int targetSamples = 256);

private:
  juce::AudioBuffer<float> buffer;
  int writeIndex = 0;
  int bufferSize = 0;
};

} // namespace flowzone
