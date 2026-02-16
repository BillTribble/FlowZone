#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <deque>
#include <vector>

namespace flowzone {

/**
 * DiskWriter: 4-Tier Resilient Recording System
 * 
 * Tier 1: Normal operation - Ring buffer writes to disk on background thread
 * Tier 2: Warning (>80% full) - Log warning + UI badge
 * Tier 3: Overflow (ring full) - Allocate temp RAM blocks up to 1GB
 * Tier 4: Critical (>1GB overflow) - Flush partial FLAC + stop recording + ERR_DISK_CRITICAL
 * 
 * Audio playback never stops, even during disk failure
 */
class DiskWriter : public juce::Thread {
public:
  enum class Tier {
    Normal = 1,      // Ring buffer OK
    Warning = 2,     // >80% full
    Overflow = 3,    // Ring full, using RAM blocks
    Critical = 4     // >1GB overflow, emergency stop
  };

  struct TierStatus {
    Tier currentTier;
    float bufferFillPercent;
    size_t overflowBytesUsed;
    juce::String statusMessage;
  };

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

  // Get current tier status
  TierStatus getTierStatus() const;

  // Thread run loop
  void run() override;

  // Callback for tier transitions (set from UI thread)
  std::function<void(Tier, const juce::String&)> onTierChange;

private:
  std::atomic<bool> recording{false};
  std::unique_ptr<juce::AudioFormatWriter> writer;
  juce::File currentFile;

  // Tier tracking
  std::atomic<Tier> currentTier{Tier::Normal};
  std::atomic<float> fillPercent{0.0f};
  
  // Ring Buffer (Tier 1)
  juce::AbstractFifo fifo{48000 * 10}; // 10 seconds buffer
  juce::AudioBuffer<float> fifoBuffer; // Circular buffer

  // Overflow RAM blocks (Tier 3)
  static constexpr size_t MAX_OVERFLOW_BYTES = 1024 * 1024 * 1024; // 1GB
  std::deque<juce::AudioBuffer<float>> overflowBlocks;
  std::atomic<size_t> overflowBytesUsed{0};
  juce::CriticalSection overflowLock;

  double sampleRate = 44100.0;
  juce::CriticalSection writerLock;

  // Tier management
  void updateTierStatus();
  void transitionToTier(Tier newTier, const juce::String& reason);
  void logTierTransition(Tier tier, float fillPercent, const juce::String& reason);
  
  // Overflow handling
  void allocateOverflowBlock(const juce::AudioBuffer<float> &buffer);
  void flushOverflowBlocks();
  void emergencyFlushAndStop();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiskWriter)
};

} // namespace flowzone
