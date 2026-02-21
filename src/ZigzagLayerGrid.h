#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A zigzag selection grid for 8 layers.
 */
class ZigzagLayerGrid : public juce::Component {
public:
  ZigzagLayerGrid() {
    for (int i = 0; i < 8; ++i) {
      auto *b = buttons.add(new juce::TextButton(juce::String(i + 1)));
      b->setClickingTogglesState(true);
      b->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A2E));
      b->setColour(juce::TextButton::buttonOnColourId,
                   juce::Colour(0xFF00CC66));
      b->onClick = [this] {
        if (onSelectionChanged)
          onSelectionChanged(getSelectionMask());
      };
      b->setToggleState(true, juce::dontSendNotification);
      addAndMakeVisible(b);
    }
  }

  void resized() override {
    auto r = getLocalBounds().reduced(10);
    int cols = 4;
    int rows = 2;
    int w = r.getWidth() / cols;
    int h = r.getHeight() / rows;

    for (int i = 0; i < 8; ++i) {
      int row = i / cols;
      int col = i % cols;
      buttons[i]->setBounds(r.getX() + col * w + 5, r.getY() + row * h + 5,
                            w - 10, h - 10);
    }
  }

  void setEnabledMask(uint8_t mask) {
    for (int i = 0; i < 8; ++i) {
      bool enabled = (mask & (1 << i)) != 0;
      buttons[i]->setEnabled(enabled);
      if (!enabled)
        buttons[i]->setToggleState(false, juce::dontSendNotification);
    }
  }

  uint8_t getSelectionMask() const {
    uint8_t mask = 0;
    for (int i = 0; i < 8; ++i) {
      if (buttons[i]->getToggleState())
        mask |= (1 << i);
    }
    return mask;
  }

  std::function<void(uint8_t)> onSelectionChanged;

private:
  juce::OwnedArray<juce::TextButton> buttons;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZigzagLayerGrid)
};
