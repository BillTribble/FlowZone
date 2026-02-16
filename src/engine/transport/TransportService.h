#pragma once

#include <JuceHeader.h>
#include <atomic>

class TransportService {
public:
  TransportService();
  ~TransportService();

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages);

  // Getters / Setters (Thread-safe)
  void setBpm(double bpm);
  double getBpm() const;

  void setLoopLengthBars(int bars);
  int getLoopLengthBars() const;

  void play();
  void pause();
  void togglePlay();
  bool isPlaying() const;

  double getBarPhase() const;    // 0.0 to 1.0
  double getPpqPosition() const; // Current beat position
  
  double getSamplesPerBar() const; // Calculate samples per bar

  void setMetronomeEnabled(bool enabled);
  bool isMetronomeEnabled() const;

  void setQuantiseEnabled(bool enabled);
  bool isQuantiseEnabled() const;

private:
  std::atomic<double> bpm{120.0};
  std::atomic<int> loopLengthBars{4};
  std::atomic<bool> playing{false};
  std::atomic<bool> metronomeEnabled{false};
  std::atomic<bool> quantiseEnabled{false};

  // Internal state (Audio Thread only)
  double sampleRate = 44100.0;
  double currentBeat = 0.0; // PPQ (Pulses Per Quarter note)

  // Click track state
  double clickPhase = 0.0;
  double clickSampleCounter = 0.0;

  // Helper
  void renderMetronome(juce::AudioBuffer<float> &buffer, int numSamples);
};
