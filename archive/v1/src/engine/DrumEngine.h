#pragma once

#include "DrumVoice.h"
#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace flowzone {
namespace engine {

/**
 * @brief Drum Engine managing 16 voices (pads).
 */
class DrumEngine {
public:
  DrumEngine();

  void prepare(double sampleRate, int samplesPerBlock);
  void process(juce::AudioBuffer<float> &buffer,
               juce::MidiBuffer &midiMessages);
  void reset();

  void setKit(const juce::String &kitName);

private:
  std::vector<std::unique_ptr<DrumVoice>> voices;
  int numPads = 16;

  DrumVoice::Type padTypes[16];

  void setupVoices();
  void triggerPad(int padIndex, float velocity);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumEngine)
};

} // namespace engine
} // namespace flowzone
