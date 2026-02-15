#pragma once
#include <JuceHeader.h>

namespace flowzone {

class CommandQueue {
public:
  CommandQueue() : fifo(2048) {}

  // Single Producer (Message Thread)
  bool push(const juce::String &command) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 + size2 > 0) {
      if (size1 > 0)
        buffer[start1] = command;
      else
        buffer[start2] = command;
      fifo.finishedWrite(1);
      return true;
    }
    return false; // Full
  }

  // Single Consumer (Audio Thread)
  bool pop(juce::String &command) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(1, start1, size1, start2, size2);
    if (size1 + size2 > 0) {
      if (size1 > 0)
        command = buffer[start1];
      else
        command = buffer[start2];
      fifo.finishedRead(1);
      return true;
    }
    return false; // Empty
  }

private:
  juce::AbstractFifo fifo;
  juce::String buffer[2048];
};

} // namespace flowzone
