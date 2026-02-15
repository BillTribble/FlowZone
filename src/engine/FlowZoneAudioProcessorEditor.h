#pragma once

#include "FlowZoneAudioProcessor.h"
#include <JuceHeader.h>

class FlowZoneAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit FlowZoneAudioProcessorEditor(FlowZoneAudioProcessor &);
  ~FlowZoneAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  FlowZoneAudioProcessor &audioProcessor;

  juce::WebBrowserComponent webView; // Uses default WebKit backend on macOS

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlowZoneAudioProcessorEditor)
};
