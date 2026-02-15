#include "Slot.h"
#include <JuceHeader.h>
#include <algorithm>

namespace flowzone {

Slot::Slot(int slotIndex) : index(slotIndex) {
  state.id = juce::String(slotIndex + 1);
  state.state = "EMPTY";
}

Slot::~Slot() {}

void Slot::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // Slots don't have a fixed size yet, they get sized when audio is set.
}

void Slot::processBlock(juce::AudioBuffer<float> &outputBuffer,
                        int numSamples) {
  if (state.state != "PLAYING" || audioData.getNumSamples() == 0)
    return;

  int sourceSamples = audioData.getNumSamples();
  int numChannels =
      std::min(audioData.getNumChannels(), outputBuffer.getNumChannels());

  int samplesToRead = numSamples;
  int outOffset = 0;

  while (samplesToRead > 0) {
    int remainingInSource = sourceSamples - playhead;
    int chunk = std::min(samplesToRead, remainingInSource);

    for (int ch = 0; ch < numChannels; ++ch) {
      outputBuffer.addFrom(ch, outOffset, audioData, ch, playhead, chunk,
                           state.volume);
    }

    playhead += chunk;
    if (playhead >= sourceSamples) {
      playhead = 0;
    }

    outOffset += chunk;
    samplesToRead -= chunk;
  }
}

void Slot::setAudioData(const juce::AudioBuffer<float> &source) {
  audioData.makeCopyOf(source);
  playhead = 0;
  state.state = "PLAYING";
}

void Slot::clear() {
  audioData.setSize(0, 0);
  state.state = "EMPTY";
  playhead = 0;
}

} // namespace flowzone
