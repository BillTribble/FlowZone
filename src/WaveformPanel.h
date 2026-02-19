#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

//==============================================================================
/// A horizontal JUCE component that renders a scrolling waveform fed from the
/// RetrospectiveBuffer. Newest audio is on the right; oldest flows off the
/// left. Four vertical division markers show 1 / 2 / 4 / 8 bar positions from
/// the right edge. Both BPM and sample rate are configurable.
class WaveformPanel : public juce::Component {
public:
  WaveformPanel();
  ~WaveformPanel() override = default;

  //==========================================================================
  /// Called from the message-thread timer. Stores a snapshot and triggers a
  /// repaint. The vector index 0 = oldest sample, last index = newest.
  void setWaveformData(const std::vector<float> &data);

  /// Update BPM for bar-line positioning. Default 120.
  void setBPM(double bpm);

  /// Must be called from prepareToPlay so bar widths in pixels are correct.
  void setSampleRate(double sampleRate);

  //==========================================================================
  // Component overrides
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  std::vector<float> waveformData; // copy owned by message thread
  double currentBPM{120.0};
  double currentSampleRate{44100.0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformPanel)
};
