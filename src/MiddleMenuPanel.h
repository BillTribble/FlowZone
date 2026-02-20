#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A central menu with "Mode" and "FX" tabs.
 */
class MiddleMenuPanel : public juce::Component {
public:
  MiddleMenuPanel();
  ~MiddleMenuPanel() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  enum class Tab { Mode, FX };
  void setActiveTab(Tab tab);

  // Wire up controls from MainComponent
  void setupModeControls(juce::Slider &gainSlider, juce::Label &gainLabel,
                         juce::Label &gainValueLabel,
                         juce::TextButton &monitorButton);

  juce::Component &getModeContainer() { return modeContainer; }
  juce::Component &getFxContainer() { return fxContainer; }

private:
  juce::TextButton modeTabButton{"MODE"};
  juce::TextButton fxTabButton{"FX"};

  juce::Component modeContainer;
  juce::Component fxContainer;

  Tab activeTab{Tab::Mode};

  // Pointers to controls owned by MainComponent
  juce::Slider *pGainSlider{nullptr};
  juce::Label *pGainLabel{nullptr};
  juce::Label *pGainValueLabel{nullptr};
  juce::TextButton *pMonitorButton{nullptr};

  void updateVisibility();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiddleMenuPanel)
};
