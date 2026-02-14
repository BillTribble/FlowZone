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
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  FlowZoneAudioProcessor &audioProcessor;

  juce::WebBrowserComponent webView;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlowZoneAudioProcessorEditor)
};
