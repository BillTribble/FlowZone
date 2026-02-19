#pragma once

#include "CrashGuard.h"
#include "FlowEngine.h"
#include "server/WebSocketServer.h"
#include <JuceHeader.h>

class FlowZoneAudioProcessorEditor; // Forward declaration (global namespace)

class FlowZoneAudioProcessor : public juce::AudioProcessor {
public:
  FlowZoneAudioProcessor();
  ~FlowZoneAudioProcessor() override;

  void init();

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  // Accessors for sub-components
  flowzone::FlowEngine &getEngine() { return engine; }
  TransportService &getTransportService() { return engine.getTransport(); }

  // Accessor for CrashGuard
  flowzone::CrashGuard &getCrashGuard() { return crashGuard; }

private:
  //==============================================================================
  flowzone::CrashGuard crashGuard;
  flowzone::FlowEngine engine;
  WebSocketServer server{50001};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlowZoneAudioProcessor)
};
