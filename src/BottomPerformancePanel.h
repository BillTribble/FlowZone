#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * BottomPerformancePanel: Dynamic lower section of the V9 Sandwich layout.
 * Displays XY Pads, Drum Pads, etc.
 */
class BottomPerformancePanel : public juce::Component {
public:
  BottomPerformancePanel() { setInterceptsMouseClicks(true, true); }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colours::black.withAlpha(0.2f));
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1.0f);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("BOTTOM PERFORMANCE AREA", getLocalBounds(),
               juce::Justification::centred);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(10);

    // Count visible children to decide layout
    int visibleCount = 0;
    for (auto *child : getChildren())
      if (child->isVisible())
        visibleCount++;

    if (visibleCount == 0)
      return;

    // Simple row layout for now
    int colW = area.getWidth() / visibleCount;
    for (auto *child : getChildren()) {
      if (child->isVisible()) {
        child->setBounds(area.removeFromLeft(colW).reduced(5));
      }
    }
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomPerformancePanel)
};
