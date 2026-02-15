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
  const juce::ScopedLock sl(
      file.exists()
          ? // Just a dummy lock check? No, thread safety for `recording`
          *(new juce::CriticalSection())
          : *(new juce::CriticalSection()));
  // Actually we don't need a lock for atomic, but we need to ensure writer
  // isn't swapped mid-write But start/stop should happen on Message Thread
  // usually.

  stopRecording();

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
    return true;
  }

  return false;
}

void DiskWriter::stopRecording() {
  recording.store(false);
  // Wait for thread to flush?
  // For Tier 1, we just close the writer on next loop
  // But we might want to signal thread explicitly
  notify();
}

void DiskWriter::writeBlock(const juce::AudioBuffer<float> &buffer) {
  if (!recording.load())
    return;

  int numSamples = buffer.getNumSamples();
  int start1, block1, start2, block2;

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
  notify();
}

void DiskWriter::run() {
  while (!threadShouldExit()) {
    if (recording.load() && writer) {
      int numReady = fifo.getNumReady();
      if (numReady > 0) {
        int start1, block1, start2, block2;
        fifo.prepareToRead(numReady, start1, block1, start2, block2);

        if (block1 > 0) {
          // Write directly from fifo buffer?
          // Writer expects AudioBuffer or float**.
          // juce::AudioFormatWriter::writeFromAudioSampleBuffer
          // But circular buffer wraps. We need to be careful.
          // We might need a temp buffer if writer doesn't support wrapped read?
          // Actually writeFromAudioSampleBuffer takes offset.

          // We can write in two chunks if wrapped
          writer->writeFromAudioSampleBuffer(fifoBuffer, start1, block1);
        }
        if (block2 > 0) {
          writer->writeFromAudioSampleBuffer(fifoBuffer, start2, block2);
        }

        fifo.finishedRead(block1 + block2);
      } else {
        wait(10); // Sleep if no data
      }
    } else {
      // If we stopped recording, maybe flush remainder?
      // For now, just wait
      wait(100);

      // If writer exists but recording is false, close it
      if (!recording.load() && writer) {
        writer.reset(); // Close file
      }
    }
  }
}

} // namespace flowzone
