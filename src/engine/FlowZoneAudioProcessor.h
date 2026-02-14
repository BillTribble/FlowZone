#pragma once

#include "server/WebSocketServer.h"
#include "transport/TransportService.h"
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
  TransportService &getTransportService() { return transportService; }

private:
  //==============================================================================
  TransportService transportService;
  WebSocketServer server{50001};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlowZoneAudioProcessor)
};
