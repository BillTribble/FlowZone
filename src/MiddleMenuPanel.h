#include "LabeledKnob.h"
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

  enum class Tab { Mode, FX, Mixer };
  void setActiveTab(Tab tab);

  std::function<void(Tab)> onTabChanged;

  // Wire up controls from MainComponent
  void setupModeControls(LabeledKnob &gainKnob, LabeledKnob &bpmKnob,
                         juce::TextButton &monitorButton);
  void setupFxControls(juce::Component &xyPad, juce::Slider &reverbSizeSlider,
                       juce::Label &reverbSizeLabel);
  void setupMixerControls();

  juce::Component &getModeContainer() { return modeContainer; }
  juce::Component &getFxContainer() { return fxContainer; }
  juce::Component &getMixerContainer() { return mixerContainer; }

private:
  juce::TextButton modeTabButton{"MODE"};
  juce::TextButton fxTabButton{"FX"};
  juce::TextButton mixerTabButton{"MIXER"};

  juce::Component modeContainer;
  juce::Component fxContainer;
  juce::Component mixerContainer;

  Tab activeTab{Tab::Mode};

  // Pointers to controls owned by MainComponent
  LabeledKnob *pGainKnob{nullptr};
  LabeledKnob *pBpmKnob{nullptr};
  juce::TextButton *pMonitorButton{nullptr};
  juce::Component *pXYPad{nullptr};
  juce::Slider *pReverbSizeSlider{nullptr};
  juce::Label *pReverbSizeLabel{nullptr};

  // Mixer settings
  juce::TextButton snapToggle{"SNAP TO BAR"};
  juce::TextButton autoQuantizeToggle{"AUTO QUANTIZE"};
  juce::Label mixerPlaceholderLabel;

  void updateVisibility();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiddleMenuPanel)
};
