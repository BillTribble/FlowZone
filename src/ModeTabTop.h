#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class ModeTabTop : public juce::Component {
public:
  ModeTabTop() {
    micModeIndicator.setText("MIC MODE", juce::dontSendNotification);
    micModeIndicator.setFont(juce::Font(24.0f, juce::Font::bold));
    micModeIndicator.setJustificationType(juce::Justification::centred);
    micModeIndicator.setColour(juce::Label::backgroundColourId,
                               juce::Colour(0xFF00CC66));
    micModeIndicator.setColour(juce::Label::textColourId, juce::Colours::black);
    addAndMakeVisible(micModeIndicator);
  }

  void resized() override {
    micModeIndicator.setBounds(getLocalBounds().reduced(20, 10));
  }

private:
  juce::Label micModeIndicator;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModeTabTop)
};
