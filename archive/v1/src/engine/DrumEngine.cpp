#include "DrumEngine.h"

namespace flowzone {
namespace engine {

DrumEngine::DrumEngine() {
  setupVoices();
  setKit("synthetic");
}

void DrumEngine::prepare(double sampleRate, int samplesPerBlock) {
  for (auto &voice : voices) {
    voice->prepare(sampleRate, samplesPerBlock);
  }
}

void DrumEngine::process(juce::AudioBuffer<float> &buffer,
                         juce::MidiBuffer &midiMessages) {
  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      int note = msg.getNoteNumber();
      // Map MIDI notes 36-51 (Standard GM Drum notes for pad controllers) to
      // pads 0-15
      if (note >= 36 && note <= 51) {
        triggerPad(note - 36, msg.getFloatVelocity());
      }
    }
  }

  for (auto &voice : voices) {
    if (voice->isActive()) {
      voice->process(buffer, 0, buffer.getNumSamples());
    }
  }
}

void DrumEngine::reset() {
  for (auto &voice : voices) {
    // No stopNote for one-shots, but we could clear active state
  }
}

void DrumEngine::setKit(const juce::String &kitName) {
  // Complete drum kit mapping for all 16 pads
  // Standard GM drum layout adapted for pads
  padTypes[0] = DrumVoice::Type::Kick;        // Pad 0: Kick
  padTypes[1] = DrumVoice::Type::Snare;       // Pad 1: Snare
  padTypes[2] = DrumVoice::Type::Hihat;       // Pad 2: Closed HH
  padTypes[3] = DrumVoice::Type::Hihat;       // Pad 3: Open HH
  padTypes[4] = DrumVoice::Type::TomLow;      // Pad 4: Low Tom
  padTypes[5] = DrumVoice::Type::TomMid;      // Pad 5: Mid Tom
  padTypes[6] = DrumVoice::Type::TomHigh;     // Pad 6: High Tom
  padTypes[7] = DrumVoice::Type::Clap;        // Pad 7: Clap
  padTypes[8] = DrumVoice::Type::Rim;         // Pad 8: Rim Shot
  padTypes[9] = DrumVoice::Type::Cowbell;     // Pad 9: Cowbell
  padTypes[10] = DrumVoice::Type::Clave;      // Pad 10: Clave
  padTypes[11] = DrumVoice::Type::Perc;       // Pad 11: Perc 1
  padTypes[12] = DrumVoice::Type::Perc;       // Pad 12: Perc 2
  padTypes[13] = DrumVoice::Type::Perc;       // Pad 13: Perc 3
  padTypes[14] = DrumVoice::Type::Perc;       // Pad 14: Perc 4
  padTypes[15] = DrumVoice::Type::Perc;       // Pad 15: Perc 5

  juce::ignoreUnused(kitName);
}

void DrumEngine::setupVoices() {
  voices.clear();
  for (int i = 0; i < numPads; ++i) {
    voices.push_back(std::make_unique<DrumVoice>());
  }
}

void DrumEngine::triggerPad(int padIndex, float velocity) {
  if (padIndex >= 0 && padIndex < numPads) {
    voices[padIndex]->trigger(velocity, padTypes[padIndex]);
  }
}

} // namespace engine
} // namespace flowzone
