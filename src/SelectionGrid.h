#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A generic selection grid of toggle buttons for Mode/Preset selection.
 */
class SelectionGrid : public juce::Component {
public:
  SelectionGrid(int rows, int cols, const juce::StringArray &labels) {
    numRows = rows;
    numCols = cols;

    for (int i = 0; i < rows * cols; ++i) {
      juce::String labelText = i < labels.size() ? labels[i] : juce::String();
      auto *b = buttons.add(new juce::TextButton(labelText));

      if (labelText.isEmpty()) {
        b->setVisible(false);
        b->setEnabled(false);
      } else {
        b->setRadioGroupId(1234); // Mutually exclusive
        b->setClickingTogglesState(true);
        b->setColour(juce::TextButton::buttonColourId,
                     juce::Colour(0xFF1A1A2E));
        b->setColour(juce::TextButton::buttonOnColourId,
                     juce::Colour(0xFF00CC66));
        b->setColour(juce::TextButton::textColourOffId,
                     juce::Colours::white.withAlpha(0.7f));
        b->setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        b->onClick = [this, i] {
          if (onSelectionChanged)
            onSelectionChanged(i);
        };
      }
      addAndMakeVisible(b);
    }

    if (buttons.size() > 0)
      buttons[0]->setToggleState(true, juce::dontSendNotification);
  }

  void resized() override {
    auto r = getLocalBounds().reduced(5);
    int w = r.getWidth() / numCols;
    int h = r.getHeight() / numRows;

    for (int i = 0; i < buttons.size(); ++i) {
      int row = i / numCols;
      int col = i % numCols;
      buttons[i]->setBounds(r.getX() + col * w + 2, r.getY() + row * h + 2,
                            w - 4, h - 4);
    }
  }

  std::function<void(int)> onSelectionChanged;

private:
  juce::OwnedArray<juce::TextButton> buttons;
  int numRows, numCols;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionGrid)
};
