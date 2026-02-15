#pragma once

#include <JuceHeader.h>
#include <atomic>

namespace flowzone {

// bd-31s: DiskWriter Tier 1
// Writes audio data to disk on a background thread
class DiskWriter : public juce::Thread {
public:
  DiskWriter();
  ~DiskWriter() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock);

  // Start recording to a specific file
  bool startRecording(const juce::File &file);

  // Stop recording
  void stopRecording();

  // Push audio block (Audio Thread Safe)
  void writeBlock(const juce::AudioBuffer<float> &buffer);

  bool isRecording() const { return recording.load(); }

  // Thread run loop
  void run() override;

private:
  std::atomic<bool> recording{false};
  std::unique_ptr<juce::AudioFormatWriter> writer;
  juce::File currentFile;

  // Ring Buffer for passing data to background thread
  // Using a simple lock + buffer for Tier 1, or AbstractFifo if avoiding locks
  // Since we are C++20, we can use a lock-free queue or just use JUCE's
  // AbstractFifo

  juce::AbstractFifo fifo{48000 * 10}; // 10 seconds buffer
  juce::AudioBuffer<float> fifoBuffer; // Circular buffer

  double sampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiskWriter)
};

} // namespace flowzone
