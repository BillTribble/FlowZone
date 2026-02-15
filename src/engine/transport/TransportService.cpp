#include "TransportService.h"

TransportService::TransportService() {}

TransportService::~TransportService() {}

void TransportService::prepareToPlay(double sr, int samplesPerBlock) {
  sampleRate = sr;
  juce::ignoreUnused(samplesPerBlock);
  clickPhase = 0.0;
  clickSampleCounter = 0.0;
}

void TransportService::processBlock(juce::AudioBuffer<float> &buffer,
                                    juce::MidiBuffer &midiMessages) {
  juce::ignoreUnused(midiMessages);

  int numSamples = buffer.getNumSamples();

  if (playing.load()) {
    double currentBpmValue = bpm.load();
    double beatsPerSecond = currentBpmValue / 60.0;
    double beatsPerSample = beatsPerSecond / sampleRate;

    // Advance transport
    currentBeat += numSamples * beatsPerSample;

    // Wrap based on loop length
    int lengthBars = loopLengthBars.load();
    double lengthBeats = lengthBars * 4.0; // Assuming 4/4 time

    while (currentBeat >= lengthBeats) {
      currentBeat -= lengthBeats;
    }
  }

  // Metronome
  if (metronomeEnabled.load() && playing.load()) {
    renderMetronome(buffer, numSamples);
  }
}

void TransportService::renderMetronome(juce::AudioBuffer<float> &buffer,
                                       int numSamples) {
  // Simple click: High pitch on beat 1, low pitch on others
  // We need to calculate beat position for each sample to be accurate,
  // or just trigger if we cross a beat boundary in this block.

  // For V1, simplest approach:
  // Check if we crossed a beat in this block.
  // Ideally sample-accurate.

  double currentBpmValue = bpm.load();
  double samplesPerBeat = (60.0 / currentBpmValue) * sampleRate;

  // We track clickSampleCounter to trigger clicks
  // Reset clickSampleCounter when it exceeds samplesPerBeat

  float *left = buffer.getWritePointer(0);
  float *right =
      buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    if (playing.load()) {
      // Increment counters
      clickSampleCounter += 1.0;
      if (clickSampleCounter >= samplesPerBeat) {
        clickSampleCounter -= samplesPerBeat;
        // Trigger click
        clickPhase = 1.0; // Start envelope
      }
    }

    if (clickPhase > 0.001) {
      // Simple sine beep with decay
      // Freq: 1000Hz (High) or 800Hz (Low)?
      // TODO: Detect bar start for accent.

      float sample =
          std::sin(clickPhase * 0.5) * clickPhase * 0.5f; // Simple tone

      // Add to buffer
      left[i] += sample;
      if (right)
        right[i] += sample;

      // Decay
      clickPhase *= 0.999f; // Fast decay
    }
  }
}

void TransportService::setBpm(double newBpm) { bpm.store(newBpm); }

double TransportService::getBpm() const { return bpm.load(); }

void TransportService::setLoopLengthBars(int bars) {
  if (bars < 1)
    bars = 1;
  loopLengthBars.store(bars);
}

int TransportService::getLoopLengthBars() const {
  return loopLengthBars.load();
}

void TransportService::play() { playing.store(true); }

void TransportService::pause() { playing.store(false); }

void TransportService::togglePlay() { playing.store(!playing.load()); }

bool TransportService::isPlaying() const { return playing.load(); }

double TransportService::getBarPhase() const {
  // Phase 0.0 to 1.0 needed for visualizer
  // currentBeat / (loopLengthBars * 4) would be full loop phase
  // User might want current BAR phase (0-1 within 4 beats)?
  // Spec §3.4: "barPhase: number; // 0.0 - 1.0"
  // Usually means position within the current measure (0-4 beats).

  double beatInBar = std::fmod(currentBeat, 4.0);
  return beatInBar / 4.0;
}

double TransportService::getPpqPosition() const { return currentBeat; }

void TransportService::setMetronomeEnabled(bool enabled) {
  metronomeEnabled.store(enabled);
}

bool TransportService::isMetronomeEnabled() const {
  return metronomeEnabled.load();
}

void TransportService::setQuantiseEnabled(bool enabled) {
  quantiseEnabled.store(enabled);
}

bool TransportService::isQuantiseEnabled() const {
  return quantiseEnabled.load();
}
