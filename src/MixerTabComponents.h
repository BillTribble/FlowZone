#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class MixerTabTop : public juce::Component {
public:
  MixerTabTop() {}
  void paint(juce::Graphics &g) override {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawText("Mixer Top Placeholder", getLocalBounds(),
               juce::Justification::centred);
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTabTop)
};

class MixerTabBottom : public juce::Component {
public:
  MixerTabBottom() {}
  void paint(juce::Graphics &g) override {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawText("Mixer Bottom Placeholder", getLocalBounds(),
               juce::Justification::centred);
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTabBottom)
};
