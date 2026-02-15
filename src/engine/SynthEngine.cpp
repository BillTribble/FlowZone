#include "SynthEngine.h"

namespace flowzone {
namespace engine {

SynthEngine::SynthEngine() {
  setupVoices();
  synth.addSound(new SynthSound());
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock) {
  synth.setCurrentPlaybackSampleRate(sampleRate);
  juce::ignoreUnused(samplesPerBlock);
}

void SynthEngine::process(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer &midiMessages) {
  synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

void SynthEngine::reset() {
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<SynthVoice *>(synth.getVoice(i))) {
      voice->stopNote(0, false);
    }
  }
}

void SynthEngine::setPreset(const juce::String &category,
                            const juce::String &presetId) {
  if (category == "bass") {
    if (presetId == "sub")
      applyPreset(0, 0.05f, 0.2f, 0.8f, 0.2f);
    else if (presetId == "gritty")
      applyPreset(1, 0.01f, 0.1f, 1.0f, 0.05f);
    else
      applyPreset(1, 0.05f, 0.2f, 0.8f, 0.2f);
  } else {
    if (presetId == "pad")
      applyPreset(0, 0.5f, 1.0f, 0.8f, 1.0f);
    else if (presetId == "lead")
      applyPreset(1, 0.01f, 0.2f, 0.5f, 0.1f);
    else
      applyPreset(3, 0.01f, 0.2f, 0.8f, 0.2f);
  }
}

void SynthEngine::setGlobalPitchRatio(float ratio) {
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<SynthVoice *>(synth.getVoice(i))) {
      voice->setPitchRatio(ratio);
    }
  }
}

void SynthEngine::setParameter(int index, float value) {
  juce::ignoreUnused(index, value);
}

void SynthEngine::setupVoices() {
  synth.clearVoices();
  for (int i = 0; i < maxVoices; ++i) {
    synth.addVoice(new SynthVoice());
  }
}

void SynthEngine::applyPreset(int oscType, float attack, float decay,
                              float sustain, float release) {
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<SynthVoice *>(synth.getVoice(i))) {
      voice->setOscillatorType(oscType);
      voice->setADSR(attack, decay, sustain, release);
    }
  }
}

} // namespace engine
} // namespace flowzone
