#pragma once
#include "Riff.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <type_traits>
#include <vector>

struct DiskWriteJob {
  juce::AudioBuffer<float> bufferCopy;
  juce::String filePath;
  double sampleRate;
};

class DiskWriter : public juce::Thread {
public:
  DiskWriter() : juce::Thread("SamsaraDiskWriter") {
    startThread(juce::Thread::Priority::background);
  }

  ~DiskWriter() override { stopThread(2000); }

  // Call this from a safe thread (e.g. Message Thread via Timer) to avoid
  // allocating on the Audio Thread.
  void queueLayerForWriting(const juce::AudioBuffer<float> &buffer,
                            const juce::String &filePath, double sampleRate) {
    juce::ScopedLock sl(queueLock);
    DiskWriteJob job;
    job.bufferCopy.makeCopyOf(buffer);
    job.filePath = filePath;
    job.sampleRate = sampleRate;
    jobQueue.push_back(std::move(job));
    notify();
  }

  void run() override {
    while (!threadShouldExit()) {
      DiskWriteJob targetJob;
      bool hasJob = false;

      {
        juce::ScopedLock sl(queueLock);
        if (!jobQueue.empty()) {
          targetJob = std::move(jobQueue.front());
          jobQueue.erase(jobQueue.begin());
          hasJob = true;
        }
      }

      if (hasJob) {
        // Write FLAC to disk
        juce::File outputFile(targetJob.filePath);
        outputFile.getParentDirectory()
            .createDirectory(); // ensure parent dir exists

        juce::FlacAudioFormat flacFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            flacFormat.createWriterFor(
                new juce::FileOutputStream(outputFile), targetJob.sampleRate,
                (unsigned int)targetJob.bufferCopy.getNumChannels(), 24, {},
                0));

        if (writer != nullptr) {
          writer->writeFromAudioSampleBuffer(
              targetJob.bufferCopy, 0, targetJob.bufferCopy.getNumSamples());
        }
      } else {
        wait(-1);
      }
    }
  }

private:
  juce::CriticalSection queueLock;
  std::vector<DiskWriteJob> jobQueue;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiskWriter)
};
