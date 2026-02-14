#pragma once

#include "FlowEngine.h"
#include "server/WebSocketServer.h"
#include <JuceHeader.h>

// Forward declarations
namespace flowzone {
class FlowZoneAudioProcessorEditor;
}

class FlowZoneAudioProcessor : public juce::AudioProcessor {
public:
  // ... (rest of public interface)

  //==============================================================================
  // Accessors for sub-components
  FlowEngine &getEngine() { return engine; }
  // Proxy transport accessor for Editor if needed
  TransportService &getTransportService() { return engine.getTransport(); }

private:
  //==============================================================================
  FlowEngine engine;
  WebSocketServer server{50001};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlowZoneAudioProcessor)
};
