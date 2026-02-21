#pragma once
#include "XYPad.h"
#include "ZigzagLayerGrid.h"
#include <juce_gui_basics/juce_gui_basics.h>

class FXTabBottom : public juce::Component {
public:
  FXTabBottom() {
    addAndMakeVisible(activeXYPad);
    addAndMakeVisible(layerGrid);
  }

  void resized() override {
    auto area = getLocalBounds();
    activeXYPad.setBounds(
        area.removeFromTop(static_cast<int>(area.getHeight() * 0.65f))
            .reduced(5));
    layerGrid.setBounds(area.reduced(5));
  }

  XYPad &getXYPad() { return activeXYPad; }
  ZigzagLayerGrid &getLayerGrid() { return layerGrid; }

private:
  XYPad activeXYPad;
  ZigzagLayerGrid layerGrid;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXTabBottom)
};
