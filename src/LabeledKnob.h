#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A composite component that couples a Rotary Slider with a Title and Value
 * label.
 */
class LabeledKnob : public juce::Component {
public:
  LabeledKnob(const juce::String &title, const juce::String &suffix = "") {
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    addAndMakeVisible(titleLabel);
    titleLabel.setText(title, juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId,
                         juce::Colours::white.withAlpha(0.6f));
    titleLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    addAndMakeVisible(valueLabel);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    valueLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));

    slider.onValueChange = [this, suffix]() {
      valueLabel.setText(juce::String(slider.getValue(), 1) + " " + suffix,
                         juce::dontSendNotification);
    };
  }

  void resized() override {
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(15));
    valueLabel.setBounds(area.removeFromBottom(18));
    slider.setBounds(area.reduced(2));
  }

  juce::Slider &getSlider() { return slider; }
  juce::Label &getTitleLabel() { return titleLabel; }
  juce::Label &getValueLabel() { return valueLabel; }

private:
  juce::Slider slider;
  juce::Label titleLabel;
  juce::Label valueLabel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledKnob)
};
