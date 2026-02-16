#include "DiskWriter.h"

namespace flowzone {

DiskWriter::DiskWriter() : juce::Thread("DiskWriterThread"), fifo(48000 * 10) {
  startThread(juce::Thread::Priority::background);
}

DiskWriter::~DiskWriter() {
  stopRecording();
  stopThread(4000);
}

void DiskWriter::prepareToPlay(double sr, int samplesPerBlock) {
  sampleRate = sr;
  // Resize FIFO buffer to handle max channels (stereo) + duration
  fifoBuffer.setSize(2, 48000 * 10);
  fifo.setTotalSize(48000 * 10);
  fifo.reset();
}

bool DiskWriter::startRecording(const juce::File &file) {
  const juce::ScopedLock sl(writerLock);
  
  stopRecording();

  // Reset tier system
  currentTier.store(Tier::Normal);
  fillPercent.store(0.0f);
  overflowBytesUsed.store(0);
  {
    const juce::ScopedLock osl(overflowLock);
    overflowBlocks.clear();
  }

  // Create directory
  file.getParentDirectory().createDirectory();

  // Delete if exists
  if (file.existsAsFile())
    file.deleteFile();

  juce::WavAudioFormat wavFormat;
  writer.reset(wavFormat.createWriterFor(new juce::FileOutputStream(file),
                                         sampleRate, 2, 24, {}, 0));

  if (writer) {
    currentFile = file;
    recording.store(true);
    logTierTransition(Tier::Normal, 0.0f, "Recording started");
    return true;
  }

  return false;
}

void DiskWriter::stopRecording() {
  recording.store(false);
  notify();
}

void DiskWriter::writeBlock(const juce::AudioBuffer<float> &buffer) {
  if (!recording.load())
    return;

  int numSamples = buffer.getNumSamples();
  int start1, block1, start2, block2;

  int numFree = fifo.getFreeSpace();
  
  // Check if block will fit in ring buffer
  if (numSamples <= numFree) {
    // Tier 1: Normal operation - write to ring buffer
    fifo.prepareToWrite(numSamples, start1, block1, start2, block2);

    if (block1 > 0) {
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        if (ch < fifoBuffer.getNumChannels()) {
          fifoBuffer.copyFrom(ch, start1, buffer, ch, 0, block1);
        }
      }
    }
    if (block2 > 0) {
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        if (ch < fifoBuffer.getNumChannels()) {
          fifoBuffer.copyFrom(ch, start2, buffer, ch, block1, block2);
        }
      }
    }

    fifo.finishedWrite(block1 + block2);
  } else {
    // Tier 3: Overflow - allocate RAM block
    allocateOverflowBlock(buffer);
  }

  updateTierStatus();
  notify();
}

void DiskWriter::updateTierStatus() {
  int numReady = fifo.getNumReady();
  int totalSize = fifo.getTotalSize();
  float percent = (float)numReady / (float)totalSize * 100.0f;
  
  fillPercent.store(percent);
  
  Tier oldTier = currentTier.load();
  Tier newTier = oldTier;
  
  size_t overflow = overflowBytesUsed.load();
  
  if (overflow > MAX_OVERFLOW_BYTES) {
    // Tier 4: Critical - emergency stop
    newTier = Tier::Critical;
  } else if (overflow > 0) {
    // Tier 3: Overflow - using RAM blocks
    newTier = Tier::Overflow;
  } else if (percent > 80.0f) {
    // Tier 2: Warning
    newTier = Tier::Warning;
  } else {
    // Tier 1: Normal
    newTier = Tier::Normal;
  }
  
  if (newTier != oldTier) {
    transitionToTier(newTier, juce::String("Buffer fill: ") + juce::String(percent, 1) + "%");
  }
}

void DiskWriter::transitionToTier(Tier newTier, const juce::String& reason) {
  currentTier.store(newTier);
  logTierTransition(newTier, fillPercent.load(), reason);
  
  // Trigger callback on message thread
  if (onTierChange) {
    juce::MessageManager::callAsync([this, newTier, reason]() {
      if (onTierChange) {
        onTierChange(newTier, reason);
      }
    });
  }
  
  // Handle critical tier
  if (newTier == Tier::Critical) {
    emergencyFlushAndStop();
  }
}

void DiskWriter::logTierTransition(Tier tier, float fillPercent, const juce::String& reason) {
  juce::String tierName;
  switch (tier) {
    case Tier::Normal: tierName = "TIER 1 (Normal)"; break;
    case Tier::Warning: tierName = "TIER 2 (Warning)"; break;
    case Tier::Overflow: tierName = "TIER 3 (Overflow)"; break;
    case Tier::Critical: tierName = "TIER 4 (Critical)"; break;
  }
  
  auto timestamp = juce::Time::getCurrentTime().toISO8601(true);
  auto overflow = overflowBytesUsed.load();
  
  DBG("DiskWriter: " << timestamp << " | " << tierName 
      << " | Fill: " << juce::String(fillPercent, 1) << "%" 
      << " | Overflow: " << juce::String(overflow / 1024.0 / 1024.0, 2) << "MB"
      << " | " << reason);
}

void DiskWriter::allocateOverflowBlock(const juce::AudioBuffer<float> &buffer) {
  const juce::ScopedLock sl(overflowLock);
  
  // Create a copy of the buffer
  juce::AudioBuffer<float> block(buffer.getNumChannels(), buffer.getNumSamples());
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    block.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
  }
  
  overflowBlocks.push_back(std::move(block));
  
  // Calculate bytes used
  size_t bytes = buffer.getNumChannels() * buffer.getNumSamples() * sizeof(float);
  overflowBytesUsed.fetch_add(bytes);
}

void DiskWriter::flushOverflowBlocks() {
  const juce::ScopedLock sl(overflowLock);
  
  if (!writer || overflowBlocks.empty())
    return;
  
  while (!overflowBlocks.empty()) {
    auto& block = overflowBlocks.front();
    
    writer->writeFromAudioSampleBuffer(block, 0, block.getNumSamples());
    
    size_t bytes = block.getNumChannels() * block.getNumSamples() * sizeof(float);
    overflowBytesUsed.fetch_sub(bytes);
    
    overflowBlocks.pop_front();
  }
}

void DiskWriter::emergencyFlushAndStop() {
  DBG("DiskWriter: EMERGENCY FLUSH - Disk write critical failure");
  
  // Try to flush what we can to FLAC (compressed)
  try {
    const juce::ScopedLock sl(writerLock);
    
    if (writer) {
      // Close current writer
      writer.reset();
      
      // Create emergency FLAC writer for remaining data
      juce::File emergencyFile = currentFile.withFileExtension(".emergency.flac");
      
      juce::FlacAudioFormat flacFormat;
      auto flacWriter = std::unique_ptr<juce::AudioFormatWriter>(
          flacFormat.createWriterFor(new juce::FileOutputStream(emergencyFile),
                                    sampleRate, 2, 24, {}, 0));
      
      if (flacWriter) {
        // Flush overflow blocks to FLAC
        const juce::ScopedLock osl(overflowLock);
        for (auto& block : overflowBlocks) {
          flacWriter->writeFromAudioSampleBuffer(block, 0, block.getNumSamples());
        }
        overflowBlocks.clear();
        overflowBytesUsed.store(0);
        
        DBG("DiskWriter: Emergency data saved to " << emergencyFile.getFullPathName());
      }
    }
  } catch (...) {
    DBG("DiskWriter: Emergency flush failed");
  }
  
  // Force stop recording
  recording.store(false);
}

DiskWriter::TierStatus DiskWriter::getTierStatus() const {
  TierStatus status;
  status.currentTier = currentTier.load();
  status.bufferFillPercent = fillPercent.load();
  status.overflowBytesUsed = overflowBytesUsed.load();
  
  switch (status.currentTier) {
    case Tier::Normal:
      status.statusMessage = "Recording OK";
      break;
    case Tier::Warning:
      status.statusMessage = "Buffer warning (>80% full)";
      break;
    case Tier::Overflow:
      status.statusMessage = juce::String("Overflow: ") + 
          juce::String(status.overflowBytesUsed / 1024.0 / 1024.0, 1) + "MB used";
      break;
    case Tier::Critical:
      status.statusMessage = "CRITICAL: Recording stopped";
      break;
  }
  
  return status;
}

void DiskWriter::run() {
  while (!threadShouldExit()) {
    if (recording.load() && writer) {
      const juce::ScopedLock sl(writerLock);
      
      // First, flush any overflow blocks if buffer has space
      if (overflowBytesUsed.load() > 0) {
        flushOverflowBlocks();
      }
      
      // Then process ring buffer
      int numReady = fifo.getNumReady();
      if (numReady > 0) {
        int start1, block1, start2, block2;
        fifo.prepareToRead(numReady, start1, block1, start2, block2);

        if (block1 > 0) {
          writer->writeFromAudioSampleBuffer(fifoBuffer, start1, block1);
        }
        if (block2 > 0) {
          writer->writeFromAudioSampleBuffer(fifoBuffer, start2, block2);
        }

        fifo.finishedRead(block1 + block2);
        updateTierStatus();
      } else {
        wait(10); // Sleep if no data
      }
    } else {
      wait(100);

      // If we stopped recording, close writer
      if (!recording.load() && writer) {
        const juce::ScopedLock sl(writerLock);
        writer.reset(); // Close file
        logTierTransition(Tier::Normal, 0.0f, "Recording stopped");
      }
    }
  }
}

} // namespace flowzone
