#pragma once
#include "SelectionGrid.h"
#include <juce_gui_basics/juce_gui_basics.h>

class FXTabTop : public juce::Component {
public:
  FXTabTop() {
    fxModeGrid = std::make_unique<SelectionGrid>(
        2, 4,
        juce::StringArray{"Dub Delay", "Space Verb", "Glitch", "Filter",
                          "Bitcrush", "Pitch", "Chorus", "Drive"});
    addAndMakeVisible(*fxModeGrid);
  }

  void resized() override { fxModeGrid->setBounds(getLocalBounds()); }

  SelectionGrid &getFXGrid() { return *fxModeGrid; }

private:
  std::unique_ptr<SelectionGrid> fxModeGrid;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXTabTop)
};
