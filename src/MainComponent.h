#pragma once
#include "LevelMeter.h"
#include "MiddleMenuPanel.h"
#include "RetrospectiveBuffer.h"
#include "Riff.h"
#include "RiffHistoryPanel.h"
#include "RiffPlaybackEngine.h"
#include "WaveformPanel.h"
#include "XYPad.h"
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
/// Main component: mic input with gain control, level meter, monitor toggle,
/// and retrospective waveform display.
class MainComponent : public juce::AudioAppComponent, private juce::Timer {
public:
  MainComponent();
  ~MainComponent() override;

  // AudioAppComponent overrides
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  // Component overrides
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  void timerCallback() override;

  // UI components
  juce::Slider gainSlider;
  juce::Label gainLabel;
  juce::Label gainValueLabel;
  LevelMeter levelMeter;
  juce::TextButton monitorButton{"MONITOR: OFF"};
  juce::Label titleLabel;
  WaveformPanel waveformPanel;
  RiffHistoryPanel riffHistoryPanel;
  MiddleMenuPanel middleMenuPanel;
  juce::Slider bpmSlider;
  juce::Label bpmLabel;
  juce::Label bpmValueLabel;

  // Reverb Controls
  juce::Slider reverbSizeSlider;
  juce::Slider reverbMixSlider;
  juce::Label reverbSizeLabel;
  juce::Label reverbMixLabel;

  // Audio pipeline
  RetrospectiveBuffer retroBuffer;
  double currentSampleRate{0.0};

  // Atomic state shared with audio thread
  std::atomic<float> gainLinear{1.0f}; // linear gain (default 0dB = 1.0)
  std::atomic<float> peakLevel{0.0f};  // peak level from audio callback
  std::atomic<bool> monitorOn{false};  // route mic to output?
  RiffHistory riffHistory;
  RiffPlaybackEngine riffEngine;
  std::atomic<double> currentBpm{120.0};

  // FX state
  juce::Reverb reverb;
  juce::Reverb::Parameters reverbParams;
  std::atomic<float> reverbRoomSize{0.5f};
  std::atomic<float> reverbWetLevel{0.3f};

  // --- Delay FX ---
  XYPad fxXYPad;
  juce::AudioBuffer<float> delayBuffer;
  int delayWritePos{0};
  std::atomic<float> delayTimeSec{0.5f};
  std::atomic<float> delayFeedback{0.5f};
  std::atomic<bool> fxEnabled{false};
  double lastCaptureTime{0.0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
