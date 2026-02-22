#pragma once
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

//==============================================================================
/// A horizontal JUCE component that renders a scrolling waveform fed from the
/// RetrospectiveBuffer. Newest audio is on the right; oldest flows off the
/// left. Four vertical division markers show 1 / 2 / 4 / 8 bar positions from
/// the right edge. Both BPM and sample rate are configurable.
class WaveformPanel : public juce::Component, private juce::Timer {
public:
  WaveformPanel();
  ~WaveformPanel() override = default;

  /// Callback called when a section is clicked. Passes the number of bars.
  std::function<void(int)> onLoopTriggered;

  /// Called from the message-thread timer. Stores a snapshot for a section.
  /// Index 0 = leftmost (8 bars), 3 = rightmost (1 bar).
  void setSectionData(int sectionIndex, const std::vector<float> &data);

  void setPPQ(double ppq);

  /// Update BPM for bar-line positioning. Default 120.
  void setBPM(double bpm);

  /// Triggers a section programmatically (0=8 bars, 3=1 bar), visually and
  /// functionally.
  void triggerSection(int section);

  /// Must be called from prepareToPlay so bar widths in pixels are correct.
  void setSampleRate(double sampleRate);

  //==========================================================================
  // Component overrides
  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;

private:
  void timerCallback() override;
  std::array<std::vector<float>, 4> sectionData; // 4 vertical strips
  int highlightedSection{-1};                    // -1 if none
  double currentBPM{120.0};
  double currentSampleRate{44100.0};
  double currentPPQ{0.0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformPanel)
};
