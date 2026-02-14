#pragma once
#include <JuceHeader.h>

namespace flowzone {

// bd-13w: Audio Thread Contract
class AudioThreadUtils {
public:
  // Assert we are on the message thread (UI/Main)
  static void assertMessageThread() { JUCE_ASSERT_MESSAGE_THREAD }

  // Custom assertion for Audio Thread
  // In a real realtime context, we might check a thread-local flag
  // For now, we can at least ensure we are NOT on the message thread
  static void assertAudioThread() {
    jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
  }

  // Forbidden operations validation (mock/concept)
  static void checkRealtimeConstraint() {
    // In a strict build, this could hook into malloc/free to panic
    // For Phase 1, we provide the hook for future instrumentation
  }
};

} // namespace flowzone
