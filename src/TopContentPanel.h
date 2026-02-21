#pragma once
#include "SelectionGrid.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * TopContentPanel: Dynamic upper section of the V9 Sandwich layout.
 * Displays Mode/Preset selection grids.
 */
class TopContentPanel : public juce::Component {
public:
  TopContentPanel() { setInterceptsMouseClicks(true, true); }

  void paint(juce::Graphics &g) override {
    // Background already handled by MainComponent or subtle here
    g.fillAll(juce::Colours::black.withAlpha(0.1f));
  }

  void resized() override {
    auto area = getLocalBounds();
    for (auto *child : getChildren()) {
      if (child->isVisible()) {
        child->setBounds(area); // Only one grid visible at a time
      }
    }
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopContentPanel)
};
