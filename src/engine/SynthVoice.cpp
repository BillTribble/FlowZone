#include "SynthVoice.h"
#include "SynthSound.h"
#include <cmath>

namespace flowzone {
namespace engine {

SynthVoice::SynthVoice() {
  adsrParams.attack = 0.01f;
  adsrParams.decay = 0.1f;
  adsrParams.sustain = 1.0f;
  adsrParams.release = 0.1f;
  adsr.setParameters(adsrParams);
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<SynthSound *>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity,
                           juce::SynthesiserSound *sound,
                           int currentPitchWheelPosition) {
  juce::ignoreUnused(sound, currentPitchWheelPosition);
  level = velocity * 0.5f;

  auto cyclesPerSecond =
      juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) * pitchRatio;
  auto cyclesPerSample = cyclesPerSecond / currentSampleRate;
  angleDelta = (float)(cyclesPerSample * 2.0 * juce::MathConstants<double>::pi);

  adsr.noteOn();
}

void SynthVoice::stopNote(float velocity, bool allowTailOff) {
  juce::ignoreUnused(velocity);
  if (allowTailOff) {
    adsr.noteOff();
  } else {
    adsr.reset();
    clearCurrentNote();
  }
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue) {
  juce::ignoreUnused(newPitchWheelValue);
}
void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue) {
  juce::ignoreUnused(controllerNumber, newControllerValue);
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                 int startSample, int numSamples) {
  if (!adsr.isActive()) {
    clearCurrentNote();
    return;
  }

  for (int sample = 0; sample < numSamples; ++sample) {
    float adsrVal = adsr.getNextSample();
    float currentSample = getNextSample() * level * adsrVal;

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
      outputBuffer.addSample(channel, startSample + sample, currentSample);
    }
  }

  if (!adsr.isActive()) {
    clearCurrentNote();
  }
}

void SynthVoice::setOscillatorType(int type) { oscType = type; }

void SynthVoice::setADSR(float a, float d, float s, float r) {
  adsrParams.attack = a;
  adsrParams.decay = d;
  adsrParams.sustain = s;
  adsrParams.release = r;
  adsr.setParameters(adsrParams);
}

void SynthVoice::setPitchRatio(float ratio) { pitchRatio = ratio; }

float SynthVoice::getNextSample() {
  float out = 0.0f;

  switch (oscType) {
  case 0: // Sine
    out = std::sin(currentAngle);
    break;
  case 1: // Saw
    out = (float)(2.0 * currentAngle / (2.0 * juce::MathConstants<double>::pi) -
                  1.0);
    break;
  case 2: // Square
    out = (currentAngle < juce::MathConstants<double>::pi) ? 1.0f : -1.0f;
    break;
  case 3: // Triangle
    out = (float)(4.0 * currentAngle / (2.0 * juce::MathConstants<double>::pi));
    if (out < 2.0f)
      out -= 1.0f;
    else
      out = 3.0f - out;
    break;
  }

  currentAngle += angleDelta;
  if (currentAngle >= 2.0 * juce::MathConstants<double>::pi) {
    currentAngle -= (float)(2.0 * juce::MathConstants<double>::pi);
  }

  return out;
}

} // namespace engine
} // namespace flowzone
