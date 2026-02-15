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
  // Basic assignment for "synthetic" kit
  // 0: Kick, 1: Snare, 2: Closed Hat, 3: Open Hat, etc.
  padTypes[0] = DrumVoice::Type::Kick;
  padTypes[1] = DrumVoice::Type::Snare;
  padTypes[2] = DrumVoice::Type::Hihat;
  padTypes[3] = DrumVoice::Type::Hihat;

  for (int i = 4; i < 16; ++i) {
    padTypes[i] = DrumVoice::Type::Perc;
  }

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
