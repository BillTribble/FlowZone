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
    // Bass presets
    if (presetId == "sub")
      applyPreset(0, 0.05f, 0.2f, 0.8f, 0.2f); // Sine, slow attack
    else if (presetId == "growl" || presetId == "gritty")
      applyPreset(1, 0.01f, 0.1f, 1.0f, 0.05f); // Saw, fast attack
    else if (presetId == "deep")
      applyPreset(0, 0.1f, 0.3f, 0.7f, 0.3f); // Sine, deeper envelope
    else if (presetId == "wobble")
      applyPreset(1, 0.001f, 0.05f, 0.9f, 0.1f); // Saw, very fast
    else if (presetId == "punch" || presetId == "808")
      applyPreset(2, 0.001f, 0.1f, 0.5f, 0.15f); // Square, punchy
    else if (presetId == "fuzz" || presetId == "reese")
      applyPreset(1, 0.01f, 0.2f, 0.8f, 0.2f); // Saw
    else if (presetId == "smooth" || presetId == "rumble")
      applyPreset(0, 0.2f, 0.4f, 0.7f, 0.3f); // Sine, smooth
    else if (presetId == "pluck-bass")
      applyPreset(1, 0.001f, 0.05f, 0.0f, 0.05f); // Saw, pluck
    else if (presetId == "acid")
      applyPreset(2, 0.001f, 0.1f, 0.3f, 0.1f); // Square, acid
    else
      applyPreset(0, 0.05f, 0.2f, 0.8f, 0.2f); // Default: Sub
  } else {
    // Notes presets
    if (presetId == "sine-bell")
      applyPreset(0, 0.01f, 0.8f, 0.3f, 1.0f); // Sine, bell envelope
    else if (presetId == "saw-lead")
      applyPreset(1, 0.001f, 0.1f, 0.7f, 0.1f); // Saw, fast attack
    else if (presetId == "square-bass")
      applyPreset(2, 0.01f, 0.2f, 0.8f, 0.2f); // Square
    else if (presetId == "triangle-pad")
      applyPreset(3, 0.5f, 1.0f, 0.8f, 1.0f); // Triangle, slow pad
    else if (presetId == "pluck")
      applyPreset(1, 0.001f, 0.05f, 0.0f, 0.05f); // Saw, pluck
    else if (presetId == "warm-pad")
      applyPreset(0, 0.8f, 1.5f, 0.7f, 1.5f); // Sine, very slow
    else if (presetId == "bright-lead")
      applyPreset(1, 0.001f, 0.05f, 0.8f, 0.1f); // Saw, bright
    else if (presetId == "soft-keys")
      applyPreset(0, 0.01f, 0.2f, 0.6f, 0.3f); // Sine, soft
    else if (presetId == "organ")
      applyPreset(1, 0.001f, 0.0f, 1.0f, 0.05f); // Saw, no decay
    else if (presetId == "ep")
      applyPreset(0, 0.001f, 0.3f, 0.5f, 0.4f); // Sine, EP-like
    else if (presetId == "choir")
      applyPreset(3, 0.6f, 1.0f, 0.9f, 1.2f); // Triangle, choir
    else if (presetId == "arp")
      applyPreset(2, 0.001f, 0.1f, 0.3f, 0.1f); // Square, arp
    else if (presetId == "pad")
      applyPreset(0, 0.5f, 1.0f, 0.8f, 1.0f); // Sine, pad
    else if (presetId == "lead")
      applyPreset(1, 0.01f, 0.2f, 0.5f, 0.1f); // Saw, lead
    else
      applyPreset(0, 0.01f, 0.2f, 0.8f, 0.2f); // Default: Sine
  }
}

void SynthEngine::setTuning(const juce::String &sclContent) {
  tuningManager.loadScl(sclContent);
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
    auto *voice = new SynthVoice();
    voice->setTuningManager(&tuningManager);
    synth.addVoice(voice);
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
