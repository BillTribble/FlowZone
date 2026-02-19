#pragma once
#include <JuceHeader.h>
#include <array>

namespace flowzone {

// Fixed size command structure for lock-free queue
struct QueuedCommand {
  static constexpr size_t kMaxCmdSize = 512;
  std::array<char, kMaxCmdSize> payload;

  void set(const juce::String &str) {
    // Copy string to buffer, truncating if necessary
    size_t len = std::min(static_cast<size_t>(str.length()), kMaxCmdSize - 1);
    std::memcpy(payload.data(), str.toRawUTF8(), len);
    payload[len] = '\0';
  }

  juce::String get() const { return juce::String::fromUTF8(payload.data()); }
};

class CommandQueue {
public:
  CommandQueue() : fifo(1024) { // Increased buffer size
    // Initialize buffer
    for (auto &cmd : buffer) {
      cmd.payload[0] = '\0';
    }
  }

  // Single Producer (Message Thread / Network Thread)
  bool push(const juce::String &command) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 + size2 > 0) {
      if (size1 > 0)
        buffer[start1].set(command);
      else
        buffer[start2].set(command);

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
        command = buffer[start1].get();
      else
        command = buffer[start2].get();

      fifo.finishedRead(1);
      return true;
    }
    return false; // Empty
  }

private:
  juce::AbstractFifo fifo;
  std::array<QueuedCommand, 1024> buffer;
};

} // namespace flowzone
