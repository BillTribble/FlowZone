#include "LabeledKnob.h"
#include "XYPad.h"
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
  void setupModeControls(LabeledKnob &gain, juce::TextButton &monitor);
  void setupFxControls(XYPad &xy, juce::Slider &reverbSize,
                       juce::Label &reverbSizeLabel);

  // New Mic Reverb controls for Mode tab
  void setupMicReverb(juce::Slider &roomSize, juce::Slider &wetLevel);
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
  juce::TextButton *pMonitorButton{nullptr};

  // Mic Reverb Pointers
  juce::Slider *pMicReverbRoomSize{nullptr};
  juce::Slider *pMicReverbWetLevel{nullptr};
  juce::Label micReverbRoomSizeLabel;
  juce::Label micReverbWetLevelLabel;
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
