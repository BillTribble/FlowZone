#pragma once
#include <JuceHeader.h>

class CommandQueue {
public:
  CommandQueue() : fifo(1024) {}

  // TODO: Implement queue logic

private:
  juce::AbstractFifo fifo;
};
