#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
  // --- Title ---
  // titleLabel.setText("FlowZone", juce::dontSendNotification);
  // titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
  // titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  // titleLabel.setJustificationType(juce::Justification::centred);
  // addAndMakeVisible(titleLabel);

  // --- Gain Slider (rotary knob) ---
  gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  gainSlider.setRange(-60.0, 40.0, 0.1); // dB range
  gainSlider.setValue(10.0);             // default 10dB
  gainSlider.setColour(juce::Slider::rotarySliderFillColourId,
                       juce::Colour(0xFF00CC66));
  gainSlider.setColour(juce::Slider::rotarySliderOutlineColourId,
                       juce::Colour(0xFF2A2A4A));
  gainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
  gainSlider.onValueChange = [this]() {
    float dB = (float)gainSlider.getValue();
    gainLinear.store(juce::Decibels::decibelsToGain(dB));

    // Update value label
    gainValueLabel.setText(juce::String(dB, 1) + " dB",
                           juce::dontSendNotification);
  };
  addAndMakeVisible(gainSlider);

  // --- Gain Label ---
  gainLabel.setText("GAIN", juce::dontSendNotification);
  gainLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  gainLabel.setColour(juce::Label::textColourId,
                      juce::Colours::white.withAlpha(0.6f));
  gainLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(gainLabel);

  // --- Gain Value Label ---
  gainValueLabel.setText("0.0 dB", juce::dontSendNotification);
  gainValueLabel.setFont(juce::FontOptions(16.0f));
  gainValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  gainValueLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(gainValueLabel);

  // --- BPM Slider ---
  bpmSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  bpmSlider.setRange(60.0, 200.0, 1.0);
  bpmSlider.setValue(120.0);
  bpmSlider.setColour(juce::Slider::rotarySliderFillColourId,
                      juce::Colour(0xFF00AAFF)); // Blue for BPM
  bpmSlider.setColour(juce::Slider::rotarySliderOutlineColourId,
                      juce::Colour(0xFF2A2A4A));
  bpmSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
  bpmSlider.onValueChange = [this]() {
    double bpm = bpmSlider.getValue();
    currentBpm.store(bpm);
    bpmValueLabel.setText(juce::String(static_cast<int>(bpm)) + " BPM",
                          juce::dontSendNotification);
    waveformPanel.setBPM(bpm);
  };
  addAndMakeVisible(bpmSlider);

  bpmLabel.setText("BPM", juce::dontSendNotification);
  bpmLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  bpmLabel.setColour(juce::Label::textColourId,
                     juce::Colours::white.withAlpha(0.6f));
  bpmLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(bpmLabel);

  bpmValueLabel.setText("120 BPM", juce::dontSendNotification);
  bpmValueLabel.setFont(juce::FontOptions(16.0f));
  bpmValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  bpmValueLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(bpmValueLabel);

  // --- FX Parameters from XY Pad ---
  fxXYPad.onXYChange = [this](float x, float y) {
    // X -> Delay Time (100ms to 800ms)
    delayTimeSec = 0.1f + (x * 0.7f);
    // Y -> Feedback (0 to 0.9)
    delayFeedback = y * 0.9f;
  };

  // --- Reverb Controls ---
  reverbSizeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  reverbSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  reverbSizeSlider.setRange(0.0, 1.0, 0.01);
  reverbSizeSlider.setValue(0.5);
  reverbSizeSlider.setColour(juce::Slider::rotarySliderFillColourId,
                             juce::Colour(0xFFCC66FF)); // Purple for Reverb
  reverbSizeSlider.setColour(juce::Slider::rotarySliderOutlineColourId,
                             juce::Colour(0xFF2A2A4A));
  reverbSizeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
  reverbSizeSlider.onValueChange = [this]() {
    reverbRoomSize.store((float)reverbSizeSlider.getValue());
  };
  addAndMakeVisible(reverbSizeSlider);

  reverbSizeLabel.setText("SIZE", juce::dontSendNotification);
  reverbSizeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  reverbSizeLabel.setColour(juce::Label::textColourId,
                            juce::Colours::white.withAlpha(0.6f));
  reverbSizeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(reverbSizeLabel);

  // Mixer slider removed as per user request (100% wet)

  // reverbMixLabel removed

  // --- FX Parameters from XY Pad ---
  fxXYPad.onXYChange = [this](float x, float y) {
    // X -> Delay Time (100ms to 800ms)
    delayTimeSec = 0.1f + (x * 0.7f);
    // Y -> Feedback (0 to 0.9)
    delayFeedback = y * 0.9f;
  };

  middleMenuPanel.onTabChanged = [this](MiddleMenuPanel::Tab tab) {
    // Tab change no longer affects fxEnabled directly, it's mouse-down on XY
    // Pad
  };

  // --- Play/Pause Button ---
  playPauseButton.setClickingTogglesState(true);
  playPauseButton.setToggleState(
      true, juce::dontSendNotification); // Default to Playing
  playPauseButton.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xFF2A2A4A));
  playPauseButton.setColour(juce::TextButton::buttonOnColourId,
                            juce::Colour(0xFF00CC66));
  playPauseButton.onClick = [this]() {
    bool playing = playPauseButton.getToggleState();
    isPlaying.store(playing);
    playPauseButton.setButtonText(playing ? "PAUSE" : "PLAY");
  };
  addAndMakeVisible(playPauseButton);

  // --- Monitor Button ---
  monitorButton.setClickingTogglesState(true);
  monitorButton.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xFF2A2A4A));
  monitorButton.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xFF00CC66));
  monitorButton.setColour(juce::TextButton::textColourOffId,
                          juce::Colours::white.withAlpha(0.7f));
  monitorButton.setColour(juce::TextButton::textColourOnId,
                          juce::Colours::black);
  monitorButton.onClick = [this]() {
    bool isOn = monitorButton.getToggleState();
    monitorOn.store(isOn);
    monitorButton.setButtonText(isOn ? "MONITOR: ON" : "MONITOR: OFF");
  };

  // --- Middle Menu ---
  addAndMakeVisible(middleMenuPanel);
  middleMenuPanel.setupModeControls(gainSlider, gainLabel, gainValueLabel,
                                    bpmSlider, bpmLabel, bpmValueLabel,
                                    monitorButton);
  middleMenuPanel.setupFxControls(fxXYPad, reverbSizeSlider, reverbSizeLabel);
  // No need to addAndMakeVisible(monitorButton) here as setupModeControls does
  // it

  // --- Waveform Panel ---
  addAndMakeVisible(waveformPanel);
  waveformPanel.setBPM(120.0);

  // --- Riff History Panel ---
  addAndMakeVisible(riffHistoryPanel);
  riffHistoryPanel.setHistory(&riffHistory);
  riffHistoryPanel.onRiffSelected = [this](const Riff &riff) {
    juce::Logger::writeToLog("Riff selected from history: " + riff.name);
    juce::AudioBuffer<float> composite;
    riff.getCompositeAudio(composite);
    riffEngine.playRiff(riff.id, composite, currentBpm.load(), true);
  };
  riffHistoryPanel.isRiffPlaying = [this](const juce::Uuid &id) {
    return riffEngine.isRiffPlaying(id);
  };
  waveformPanel.onLoopTriggered = [this](int bars) {
    juce::Logger::writeToLog(
        "Retrospective Loop Triggered: " + juce::String(bars) + " bars");

    if (currentSampleRate <= 0.0)
      return;

    const double bpm = currentBpm.load();
    const double framesPerBar = currentSampleRate * (60.0 / bpm) * 4.0;
    const int numFrames = static_cast<int>(framesPerBar * bars);

    const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const double timeSinceLastCapture = now - lastCaptureTime;
    lastCaptureTime = now;

    Riff *lastRiff = riffHistory.getLastRiff();
    // Only merge if it's the same bar length AND captured recently
    if (lastRiff != nullptr && lastRiff->bars == bars && lastRiff->layers < 8 &&
        timeSinceLastCapture < 5.0) {
      juce::AudioBuffer<float> layerAudio;
      retroBuffer.getAudioRegion(layerAudio, numFrames);
      lastRiff->merge(layerAudio);
      riffHistory.signalUpdate();

      juce::AudioBuffer<float> composite;
      lastRiff->getCompositeAudio(composite);
      riffEngine.playRiff(lastRiff->id, composite, bpm, true);
      riffHistoryPanel.repaint();
      juce::Logger::writeToLog("Merged into existing riff. Layers: " +
                               juce::String(lastRiff->layers));
    } else {
      Riff newRiff;
      newRiff.bpm = bpm;
      newRiff.bars = bars;
      newRiff.name = "Riff " + juce::String(riffHistory.size() + 1);
      newRiff.layers = 1;

      // Capture the audio from the buffer
      juce::AudioBuffer<float> capturedAudio;
      retroBuffer.getAudioRegion(capturedAudio, numFrames);
      newRiff.merge(capturedAudio);

      juce::Logger::writeToLog("Captured New Riff: " + newRiff.name + " (" +
                               juce::String(numFrames) + " samples)");

      // Add to history
      const auto &ref = riffHistory.addRiff(std::move(newRiff));
      juce::AudioBuffer<float> composite;
      ref.getCompositeAudio(composite);
      riffEngine.playRiff(ref.id, composite, bpm, true);
      riffHistoryPanel.repaint();
    }
  };

  // --- Audio Setup ---
  // Request 2 inputs and 2 outputs — mic feeds left channel primarily.
  setAudioChannels(2, 2);

  // --- Timer for UI update at ~30fps ---
  startTimerHz(30);

  setSize(400, 750);
}

MainComponent::~MainComponent() { shutdownAudio(); }

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  currentSampleRate = sampleRate;
  retroBuffer.prepare(sampleRate, 60); // 60 seconds storage
  riffEngine.prepare(sampleRate, samplesPerBlockExpected);
  waveformPanel.setSampleRate(sampleRate);

  // FX: Allocate 2 seconds for delay
  delayBuffer.setSize(2, static_cast<int>(sampleRate * 2.0));
  delayBuffer.clear();
  delayWritePos = 0;

  reverb.setSampleRate(sampleRate);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  auto *device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr) {
    bufferToFill.clearActiveBufferRegion();
    return;
  }

  auto *buffer = bufferToFill.buffer;
  int numSamples = bufferToFill.numSamples;
  int numInputChannels = buffer->getNumChannels();

  // 1. Process Input Gain
  float gain = gainLinear.load();
  buffer->applyGain(0, numSamples, gain); // Simplified gain on input

  // Calculate peak for level meter (monitor input)
  float inputPeak = buffer->getMagnitude(0, numSamples);
  peakLevel.store(inputPeak);

  // 2. Prepare workspace for mixing (using a temporary copy of input)
  juce::AudioBuffer<float> inputCopy;
  inputCopy.makeCopyOf(*buffer);

  // Clear output buffer to start mixing
  bufferToFill.clearActiveBufferRegion();

  // Add Riffs if playing
  if (isPlaying.load()) {
    riffEngine.processNextBlock(*buffer, currentBpm.load());
  }

  // Add Input to output (Monitor)
  if (monitorOn.load()) {
    for (int ch = 0; ch < buffer->getNumChannels(); ++ch) {
      buffer->addFrom(ch, 0, inputCopy, ch, 0, numSamples);
    }
  }

  // 3. Define a shared FX processing lambda
  auto applyGlobalFX = [&](juce::AudioBuffer<float> &targetBuffer) {
    if (fxXYPad.isMouseButtonDown()) {
      // --- Delay FX ---
      for (int ch = 0; ch < targetBuffer.getNumChannels(); ++ch) {
        float *channelData = targetBuffer.getWritePointer(ch);
        float *delayData = delayBuffer.getWritePointer(ch);
        int delaySize = delayBuffer.getNumSamples();

        float dTime = delayTimeSec.load();
        float dFeedback = delayFeedback.load();
        int delayInSamples = static_cast<int>(currentSampleRate * dTime);

        for (int i = 0; i < numSamples; ++i) {
          int readPos =
              (delayWritePos - delayInSamples + i + delaySize) % delaySize;
          float delaySample = delayData[readPos];

          // 100% wet when active
          float currentSample = channelData[i];
          channelData[i] = delaySample;

          delayData[(delayWritePos + i) % delaySize] =
              currentSample + (delaySample * dFeedback);
        }
      }

      // --- Reverb FX ---
      reverbParams.roomSize = reverbRoomSize.load();
      reverbParams.wetLevel = 1.0f; // 100% wet when active
      reverbParams.dryLevel = 0.0f;
      reverb.setParameters(reverbParams);

      if (targetBuffer.getNumChannels() >= 2) {
        reverb.processStereo(targetBuffer.getWritePointer(0),
                             targetBuffer.getWritePointer(1), numSamples);
      } else {
        reverb.processMono(targetBuffer.getWritePointer(0), numSamples);
      }
    }
  };

  // 4. Final Processing & Looper Feed
  // We want 'looperMix' to capture everything: Input + Riffs + FX
  // regardless of monitoring status.
  juce::AudioBuffer<float> looperMix;
  looperMix.makeCopyOf(inputCopy); // Always has input + gain
  if (isPlaying.load()) {
    riffEngine.processNextBlock(looperMix, currentBpm.load());
  }

  // Apply FX to both (speaker buffer and looper buffer)
  applyGlobalFX(*buffer);   // Already has Riffs + (Input if monitorOn)
  applyGlobalFX(looperMix); // Has Input + Riffs

  // Update delay write position once after all processing
  if (fxXYPad.isMouseButtonDown()) {
    delayWritePos = (delayWritePos + numSamples) % delayBuffer.getNumSamples();
  }

  // Push looperMix (always has input) to retrospective buffer
  retroBuffer.pushBlock(looperMix);
}

void MainComponent::releaseResources() {
  // Nothing to release
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // Dark premium background
  g.fillAll(juce::Colour(0xFF0F0F23));

  // Subtle gradient overlay
  juce::ColourGradient bgGradient(juce::Colour(0xFF16162B), 0, 0,
                                  juce::Colour(0xFF0A0A1A), 0,
                                  (float)getHeight(), false);
  g.setGradientFill(bgGradient);
  g.fillRect(getLocalBounds());

  // Subtle separator line below title
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.fillRect(20, 50, getWidth() - 40, 1);
}

void MainComponent::resized() {
  auto area = getLocalBounds().reduced(20);

  // Header Area (Top 40px)
  auto headerArea = area.removeFromTop(40);

  // Level meter in the middle
  levelMeter.setHorizontal(true);
  levelMeter.setBounds(headerArea.withSizeKeepingCentre(
      std::min(headerArea.getWidth() - 100, 300), 12));

  // Play/Pause button on the right
  playPauseButton.setBounds(headerArea.removeFromRight(80).reduced(2, 5));

  area.removeFromTop(10); // spacing

  // Main content area - takes up the middle section above the bottom panels
  auto contentArea = area.removeFromTop(area.getHeight() - 200);

  // Middle Menu fills the content area
  middleMenuPanel.setBounds(contentArea.reduced(5, 0));

  // --- Waveform Panel at the bottom, full width, 120px ---
  auto bottomArea = getLocalBounds().removeFromBottom(200);
  waveformPanel.setBounds(bottomArea.removeFromTop(120));
  riffHistoryPanel.setBounds(bottomArea);
}

//==============================================================================
void MainComponent::timerCallback() {
  // Read peak from audio thread (atomic) and update the meter
  levelMeter.setLevel(peakLevel.load());

  // Feed waveform panel sections: 8, 4, 2, 1 bars at current BPM
  if (currentSampleRate > 0.0) {
    const double bpm = currentBpm.load();
    const int panelW = std::max(waveformPanel.getWidth(), 1);
    const int framesPerBar =
        static_cast<int>(currentSampleRate * (60.0 / bpm) * 4.0);

    // BPM calculation (default 120)
    const int sectionW = std::max(panelW / 4, 1);

    const int bars[] = {8, 4, 2, 1};
    for (int i = 0; i < 4; ++i) {
      const int numFrames = static_cast<int>(framesPerBar * bars[i]);
      auto sectionData = retroBuffer.getWaveformData(numFrames, sectionW);
      waveformPanel.setSectionData(i, sectionData);
    }
  }
}
