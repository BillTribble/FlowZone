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
  void setupMicReverb();
  void setupMixerControls();

private:
  juce::TextButton modeTabButton{"MODE"};
  juce::TextButton soundTabButton{"SOUND"};
  juce::TextButton fxTabButton{"FX"};
  juce::TextButton mixerTabButton{"MIXER"};

  Tab activeTab{Tab::Mode};

  // Pointers to controls owned by MainComponent
  juce::Label micReverbRoomSizeLabel;
  juce::Label micReverbWetLevelLabel;

  // Mixer settings
  juce::TextButton snapToggle{"SNAP TO BAR"};
  juce::TextButton autoQuantizeToggle{"AUTO QUANTIZE"};
  juce::Label mixerPlaceholderLabel;

  void updateVisibility();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiddleMenuPanel)
};
