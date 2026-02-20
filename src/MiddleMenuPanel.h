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

  std::function<void(Tab)> onTabChanged;

  // Wire up controls from MainComponent
  void setupModeControls(juce::Slider &gainSlider, juce::Label &gainLabel,
                         juce::Label &gainValueLabel, juce::Slider &bpmSlider,
                         juce::Label &bpmLabel, juce::Label &bpmValueLabel,
                         juce::TextButton &monitorButton);
  void setupFxControls(juce::Component &xyPad, juce::Slider &reverbSizeSlider,
                       juce::Slider &reverbMixSlider,
                       juce::Label &reverbSizeLabel,
                       juce::Label &reverbMixLabel);

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
  juce::Slider *pBpmSlider{nullptr};
  juce::Label *pBpmLabel{nullptr};
  juce::Label *pBpmValueLabel{nullptr};
  juce::Component *pXYPad{nullptr};
  juce::Slider *pReverbSizeSlider{nullptr};
  juce::Slider *pReverbMixSlider{nullptr};
  juce::Label *pReverbSizeLabel{nullptr};
  juce::Label *pReverbMixLabel{nullptr};

  void updateVisibility();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiddleMenuPanel)
};
