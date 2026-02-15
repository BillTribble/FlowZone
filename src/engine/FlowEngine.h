#include "CommandDispatcher.h"
#include "CommandQueue.h"
#include "CrashGuard.h"
#include "session/SessionStateManager.h"
#include "state/StateBroadcaster.h"
#include "transport/TransportService.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-3uw: FlowEngine Skeleton
// Coordinates DSP graph, loopers, and transport
class FlowEngine {
public:
  FlowEngine();
  ~FlowEngine();

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages);

  TransportService &getTransport() { return transport; }
  CommandDispatcher &getDispatcher() { return dispatcher; }
  StateBroadcaster &getBroadcaster() { return broadcaster; }
  SessionStateManager &getSessionManager() { return sessionManager; }
  CommandQueue &getCommandQueue() { return commandQueue; }

private:
  TransportService transport;
  CommandDispatcher dispatcher;
  StateBroadcaster broadcaster;
  SessionStateManager sessionManager;
  CrashGuard crashGuard;
  CommandQueue commandQueue;

  void processCommands();
};

} // namespace flowzone
