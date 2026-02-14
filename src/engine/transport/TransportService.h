#pragma once

#include <JuceHeader.h>

class TransportService {
public:
  TransportService();
  ~TransportService();

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages);

  // Getters / Setters
  void setBpm(double bpm);
  double getBpm() const;

  void play();
  void pause();
  bool isPlaying() const;

private:
  double currentBpm = 120.0;
  bool playing = false;
  double sampleRate = 44100.0;
};
