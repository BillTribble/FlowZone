/**
 * PluginHostApp - Isolated VST3 Plugin Host Process
 * 
 * This is a separate executable that runs plugins in isolation.
 * Communication with the main FlowZone process happens via shared memory.
 * 
 * Architecture:
 * - Each manufacturer gets its own host process
 * - Shared memory ring buffers for audio I/O
 * - Heartbeat protocol for crash detection
 * - Clean crash isolation per manufacturer
 * 
 * Build: Configure as Console Application target in Projucer
 */

#include <JuceHeader.h>
#include "PluginHostProcess.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: PluginHostApp <manufacturer_id>" << std::endl;
        return 1;
    }
    
    juce::String manufacturerId(argv[1]);
    
    juce::ScopedJuceInitialiser_GUI initialiser;
    
    flowzone::PluginHostProcess host(manufacturerId);
    
    if (!host.initialize()) {
        std::cerr << "Failed to initialize plugin host for: " 
                  << manufacturerId << std::endl;
        return 1;
    }
    
    DBG("PluginHostApp started for manufacturer: " << manufacturerId);
    
    // Run until signaled to stop
    host.run();
    
    DBG("PluginHostApp shutting down for manufacturer: " << manufacturerId);
    
    return 0;
}
