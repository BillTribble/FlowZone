#pragma once
#include <JuceHeader.h>

namespace flowzone {

class CrashGuard {
public:
  CrashGuard() {
    sentinelFile =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("FlowZone_CrashGuard_Sentinel");
  }

  void markActive() { sentinelFile.create(); }

  void markClean() {
    if (sentinelFile.exists()) {
      sentinelFile.deleteFile();
    }
  }

  bool wasCrashed() const { return sentinelFile.exists(); }

private:
  juce::File sentinelFile;
};

} // namespace flowzone
