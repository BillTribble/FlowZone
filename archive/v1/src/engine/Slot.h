#pragma once

#include "state/AppState.h"
#include <JuceHeader.h>
#include <string>

namespace flowzone {

/**
 * bd-338: Loop Slot implementation.
 * Manages the audio buffer for a single loop and its playback state.
 */
class Slot {
public:
  Slot(int slotIndex);
  ~Slot();

  void prepareToPlay(double sampleRate, int samplesPerBlock);

  /**
   * Processes a block of audio.
   * If state is PLAYING, sums its buffer into the provided output buffer.
   * If state is RECORDING, captures input into its buffer (handled by
   * FlowEngine/RetroBuffer).
   */
  void processBlock(juce::AudioBuffer<float> &outputBuffer, int numSamples);

  /**
   * Sets the audio data for this slot.
   * Used when capturing from RetrospectiveBuffer or during Auto-Merge.
   */
  void setAudioData(const juce::AudioBuffer<float> &source);

  /**
   * Clears the slot and sets state to EMPTY.
   */
  void clear();

  SlotState &getState() { return state; }
  const SlotState &getState() const { return state; }

  void setVolume(float newVolume) { state.volume = newVolume; }
  void setMuted(bool muted) { state.muted = muted; }
  bool isMuted() const { return state.muted; }
  void setState(const juce::String &newState) { state.state = newState; }

  bool isFull() const { return state.state != "EMPTY"; }
  int getLengthInBars() const { return state.loopLengthBars; }

private:
  int index;
  SlotState state;
  juce::AudioBuffer<float> audioData;
  int playhead = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Slot)
};

} // namespace flowzone
