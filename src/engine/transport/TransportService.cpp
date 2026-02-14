#include "TransportService.h"

TransportService::TransportService() {}

TransportService::~TransportService() {}

void TransportService::prepareToPlay(double sr, int samplesPerBlock) {
  sampleRate = sr;
  juce::ignoreUnused(samplesPerBlock);
}

void TransportService::processBlock(juce::AudioBuffer<float> &buffer,
                                    juce::MidiBuffer &midiMessages) {
  juce::ignoreUnused(buffer, midiMessages);
  // TODO: Implement transport logic
}

void TransportService::setBpm(double bpm) { currentBpm = bpm; }

double TransportService::getBpm() const { return currentBpm; }

void TransportService::play() { playing = true; }

void TransportService::pause() { playing = false; }

bool TransportService::isPlaying() const { return playing; }
