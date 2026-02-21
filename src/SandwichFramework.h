#pragma once
#include "MiddleMenuPanel.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SandwichFramework: A structural component that manages a Top, Middle (Tabs),
 * and Bottom area. It facilitates swapping the Top and Bottom components based
 * on tab selection.
 */
class SandwichFramework : public juce::Component {
public:
  SandwichFramework(MiddleMenuPanel &tabs) : middleMenuPanel(tabs) {
    addAndMakeVisible(middleMenuPanel);
  }

  void setTopComponent(juce::Component *newTop) {
    if (topComponent != newTop) {
      if (topComponent != nullptr)
        removeChildComponent(topComponent);

      topComponent = newTop;

      if (topComponent != nullptr)
        addAndMakeVisible(topComponent);

      resized();
    }
  }

  void setBottomComponent(juce::Component *newBottom) {
    if (bottomComponent != newBottom) {
      if (bottomComponent != nullptr)
        removeChildComponent(bottomComponent);

      bottomComponent = newBottom;

      if (bottomComponent != nullptr)
        addAndMakeVisible(bottomComponent);

      resized();
    }
  }

  void resized() override {
    auto area = getLocalBounds();

    // Header/Title area is usually outside this framework or handled by
    // MainComponent But if we want the full "sandwich" here:

    // 1. Top Section (Flexible height)
    // 2. Tab Bar (Fixed height)
    // 3. Bottom Section (Fixed height usually)

    // In the current layout:
    // Top: ~260px (flexible)
    // Tabs: 40px (fixed)
    // Bottom: 240px (fixed)

    auto bottomArea = area.removeFromBottom(240);
    auto tabArea = area.removeFromBottom(40);
    auto topArea = area; // Remaining space

    if (topComponent != nullptr)
      topComponent->setBounds(topArea);

    middleMenuPanel.setBounds(tabArea);

    if (bottomComponent != nullptr)
      bottomComponent->setBounds(bottomArea);
  }

private:
  MiddleMenuPanel &middleMenuPanel;
  juce::Component *topComponent = nullptr;
  juce::Component *bottomComponent = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandwichFramework)
};
