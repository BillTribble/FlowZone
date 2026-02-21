#pragma once
#include "BottomPerformancePanel.h"
#include "FlowZoneLogger.h"
#include "LabeledKnob.h"
#include "LevelMeter.h"
#include "MiddleMenuPanel.h"
#include "RetrospectiveBuffer.h"
#include "Riff.h"
#include "RiffHistoryPanel.h"
#include "RiffPlaybackEngine.h"
#include "SelectionGrid.h"
#include "StandaloneFXEngine.h"
#include "TopContentPanel.h"
#include "WaveformPanel.h"
#include "XYPad.h"
#include "ZigzagLayerGrid.h"
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
  void updateLayoutForTab(MiddleMenuPanel::Tab tab);

private:
  void timerCallback() override;

  // UI components
  LabeledKnob gainKnob{"GAIN", "dB"};
  LevelMeter levelMeter;
  juce::TextButton monitorButton{"MONITOR: OFF"};
  juce::Label titleLabel;
  juce::TextButton playPauseButton{
      "PAUSE"}; // Starts in playing mode (PAUSE is the action)
  WaveformPanel waveformPanel;
  RiffHistoryPanel riffHistoryPanel;
  MiddleMenuPanel middleMenuPanel;
  TopContentPanel topContentPanel;
  BottomPerformancePanel bottomPerformancePanel;
  XYPad activeXYPad;
  ZigzagLayerGrid layerGrid;
  std::unique_ptr<SelectionGrid> instrumentModeGrid;
  std::unique_ptr<SelectionGrid> soundPresetGrid;
  std::unique_ptr<SelectionGrid> fxModeGrid;

  // BPM Header Display (Custom component for drag-to-edit)
  struct BpmDisplay : public juce::Component {
    MainComponent &owner;
    BpmDisplay(MainComponent &o) : owner(o) {
      setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    void paint(juce::Graphics &g) override {
      g.setColour(juce::Colours::white);
      g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
      g.drawText(juce::String(owner.currentBpm.load(), 1) + " BPM",
                 getLocalBounds(), juce::Justification::centred, false);
    }
    void mouseDrag(const juce::MouseEvent &e) override {
      float delta = -e.getDistanceFromDragStartY() * 0.2f;
      owner.updateBpm(owner.currentBpm.load() + delta);
      LOG_ACTION("Engine",
                 "BPM Adjusted: " + juce::String(owner.currentBpm.load(), 1));
    }
  } bpmDisplay{*this};

  // New Mic Reverb UI (for the Mode tab)
  juce::Slider micReverbSizeSlider;
  juce::Slider micReverbMixSlider;
  juce::Label micReverbSizeLabel;
  juce::Label micReverbMixLabel;

  // Audio pipeline
  RetrospectiveBuffer retroBuffer;
  double currentSampleRate{0.0};

  // Atomic state shared with audio thread
  std::atomic<float> gainLinear{1.0f}; // linear gain
  std::atomic<float> peakLevel{0.0f};  // peak level from audio callback
  std::atomic<bool> monitorOn{false};  // route mic to output?
  std::atomic<bool> isPlaying{true};   // play riffs?

  void updateBpm(double newBpm);
  RiffHistory riffHistory;
  RiffPlaybackEngine riffEngine;
  std::atomic<double> currentBpm{120.0};
  MiddleMenuPanel::Tab activeTab{MiddleMenuPanel::Tab::Mode};

  // --- Mic Reverb (Separate from Master FX) ---
  juce::Reverb micReverb;
  juce::Reverb::Parameters micReverbParams;
  std::atomic<float> micReverbRoomSize{0.5f};
  std::atomic<float> micReverbWetLevel{0.0f}; // Default off

  // --- V9 Selective FX Engine ---
  StandaloneFXEngine fxEngine;
  std::atomic<uint8_t> selectedLayers{0}; // Bitmask for layers 1-8

  // --- Scratch Buffers (Pre-allocated to avoid allocations in audio thread)
  // ---
  juce::AudioBuffer<float> inputCopyBuffer;
  juce::AudioBuffer<float> looperMixBuffer;
  juce::AudioBuffer<float> riffOutputBuffer;

  // --- Settings ---
  juce::TextButton settingsButton{"SETTINGS"};
  std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

  double lastCaptureTime{0.0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
