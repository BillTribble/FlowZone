#pragma once
#include "LabeledKnob.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ModeTabBottom : public juce::Component {
public:
  ModeTabBottom() {
    addAndMakeVisible(gainKnob);

    monitorButton.setClickingTogglesState(true);
    monitorButton.setColour(juce::TextButton::buttonOnColourId,
                            juce::Colour(0xFF00CC66));
    monitorButton.setColour(juce::TextButton::textColourOffId,
                            juce::Colours::white.withAlpha(0.6f));
    monitorButton.setColour(juce::TextButton::textColourOnId,
                            juce::Colours::black);
    addAndMakeVisible(monitorButton);

    micReverbSizeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    micReverbSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(micReverbSizeSlider);

    micReverbMixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    micReverbMixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(micReverbMixSlider);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(10);

    gainKnob.setBounds(area.removeFromLeft(100).reduced(10));

    auto btnArea = area.removeFromTop(40);
    monitorButton.setBounds(btnArea.reduced(5, 2));

    micReverbSizeSlider.setBounds(
        area.removeFromTop(area.getHeight() / 2).reduced(5));
    micReverbMixSlider.setBounds(area.reduced(5));
  }

  LabeledKnob &getGainKnob() { return gainKnob; }
  juce::TextButton &getMonitorButton() { return monitorButton; }
  juce::Slider &getReverbSizeSlider() { return micReverbSizeSlider; }
  juce::Slider &getReverbMixSlider() { return micReverbMixSlider; }

private:
  LabeledKnob gainKnob{"GAIN", "dB"};
  juce::TextButton monitorButton{"MONITOR: OFF"};
  juce::Slider micReverbSizeSlider;
  juce::Slider micReverbMixSlider;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModeTabBottom)
};
