#pragma once
#include "SelectionGrid.h"
#include <juce_gui_basics/juce_gui_basics.h>

class FXTabTop : public juce::Component {
public:
  FXTabTop() {
    juce::StringArray coreFX = {"Lowpass",   "Highpass",  "Reverb",  "Gate",
                                "Keymasher", "Gate Trip", "Distort", "Delay",
                                "Comb",      "Pitchmod",  "Smudge",  "Dub Dly"};
    juce::StringArray infFX = {"Buzz",      "Ringmod", "Bitcrush", "Degrader",
                               "Multicomb", "Freezer", "Zap Dly",  "TrGate",
                               "",          "",        "",         ""};

    grid1 = std::make_unique<SelectionGrid>(3, 4, coreFX);
    grid2 = std::make_unique<SelectionGrid>(3, 4, infFX);

    grid1->onSelectionChanged = [this](int idx) {
      if (onFXSelected)
        onFXSelected(idx);
    };
    grid2->onSelectionChanged = [this](int idx) {
      if (onFXSelected)
        onFXSelected(idx + 12);
    };

    addAndMakeVisible(*grid1);
    addChildComponent(*grid2);

    addMouseListener(this, true); // Catch drags from buttons for swiping
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromBottom(20); // space for dots
    grid1->setBounds(bounds);
    grid2->setBounds(bounds);
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds();
    float midX = bounds.getWidth() / 2.0f;
    float y = bounds.getBottom() - 15.0f;

    // Draw dots
    g.setColour(pageIndex == 0 ? juce::Colours::white
                               : juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(midX - 12.0f, y, 8.0f, 8.0f);

    g.setColour(pageIndex == 1 ? juce::Colours::white
                               : juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(midX + 4.0f, y, 8.0f, 8.0f);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    dragStartX = e.getScreenPosition().x;
  }

  void mouseUp(const juce::MouseEvent &e) override {
    float dx = e.getScreenPosition().x - dragStartX;
    if (dx < -40.0f && pageIndex == 0) {
      setPage(1);
    } else if (dx > 40.0f && pageIndex == 1) {
      setPage(0);
    } else if (std::abs(dx) < 10.0f) {
      // Handle dot clicks
      auto relativePos = getLocalPoint(e.eventComponent, e.position);
      auto bounds = getLocalBounds();
      float y = bounds.getBottom() - 20.0f;
      if (relativePos.y > y) {
        float midX = bounds.getWidth() / 2.0f;
        if (relativePos.x < midX)
          setPage(0);
        else
          setPage(1);
      }
    }
  }

  void setPage(int newPage) {
    if (pageIndex == newPage)
      return;
    pageIndex = newPage;
    grid1->setVisible(pageIndex == 0);
    grid2->setVisible(pageIndex == 1);
    repaint();
  }

  std::function<void(int)> onFXSelected;

private:
  std::unique_ptr<SelectionGrid> grid1;
  std::unique_ptr<SelectionGrid> grid2;
  int pageIndex = 0;
  float dragStartX = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXTabTop)
};
