#pragma once
#include "FXTabBottom.h"
#include "FXTabTop.h"
#include "FlowZoneLogger.h"
#include "LevelMeter.h"
#include "MixerTabComponents.h"
#include "ModeTabBottom.h"
#include "ModeTabTop.h"
#include "RetrospectiveBuffer.h"
#include "Riff.h"
#include "RiffHistoryPanel.h"
#include "RiffPlaybackEngine.h"
#include "SandwichFramework.h"
#include "StandaloneFXEngine.h"
#include "WaveformPanel.h"
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
  void setupModeTabLogic();
  void setupFXTabLogic();

  // --- Dynamic Layout Helpers ---
  void updateBpm(double newBpm);

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
    }
  };

  // UI components
  juce::TextButton settingsButton{"SETTINGS"};
  juce::TextButton playPauseButton{"PAUSE"};
  BpmDisplay bpmDisplay{*this};
  LevelMeter levelMeter;

  WaveformPanel waveformPanel;
  RiffHistoryPanel riffHistoryPanel;
  MiddleMenuPanel middleMenuPanel;

  // Framework & Modular Components
  SandwichFramework framework{middleMenuPanel};

  ModeTabTop modeTop;
  ModeTabBottom modeBottom;

  FXTabTop fxTop;
  FXTabBottom fxBottom;

  MixerTabTop mixerTop;
  MixerTabBottom mixerBottom;

  // Audio pipeline
  RetrospectiveBuffer retroBuffer;
  double currentSampleRate{0.0};

  // Atomic state shared with audio thread
  std::atomic<float> gainLinear{1.0f}; // linear gain
  std::atomic<float> peakLevel{0.0f};  // peak level from audio callback
  std::atomic<bool> monitorOn{false};  // route mic to output?
  std::atomic<bool> isPlaying{true};   // play riffs?

  RiffHistory riffHistory;
  RiffPlaybackEngine riffEngine;
  std::atomic<double> playbackPosition{0.0};
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

  // --- Scratch Buffers ---
  juce::AudioBuffer<float> inputCopyBuffer;
  juce::AudioBuffer<float> looperMixBuffer;
  juce::AudioBuffer<float> riffOutputBuffer;

  std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
  double lastCaptureTime{0.0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
