#pragma once

#include <JuceHeader.h>

namespace flowzone {
namespace engine {

/**
 * @brief Standard Synth Sound for the JUCE synthesiser.
 * Accepts all midi channels and notes for now.
 */
class SynthSound : public juce::SynthesiserSound {
public:
  SynthSound() {}
  bool appliesToNote(int) override { return true; }
  bool appliesToChannel(int) override { return true; }
};

} // namespace engine
} // namespace flowzone
