#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
  // --- Title ---
  // titleLabel.setText("FlowZone", juce::dontSendNotification);
  // titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
  // titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  // titleLabel.setJustificationType(juce::Justification::centred);
  // addAndMakeVisible(titleLabel);

  // --- Gain Knob ---
  auto &gainSlider = gainKnob.getSlider();
  gainSlider.setRange(-60.0, 12.0, 0.1);
  gainSlider.setValue(10.0); // Initial Gain is 10 dB as requested
  gainSlider.onValueChange = [this, &gainSlider]() {
    float dB = static_cast<float>(gainSlider.getValue());
    gainLinear.store(juce::Decibels::decibelsToGain(dB));
  };
  addAndMakeVisible(gainKnob);

  // --- BPM Knob ---
  auto &bpmSlider = bpmKnob.getSlider();
  bpmSlider.setRange(40.0, 240.0, 1.0);
  bpmSlider.setValue(120.0);
  bpmSlider.onValueChange = [this, &bpmSlider]() {
    double val = bpmSlider.getValue();
    currentBpm.store(val);
    waveformPanel.setBPM(val);
  };
  addAndMakeVisible(bpmKnob);

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
                          juce::Colour(0xFF1A1A2E));
  monitorButton.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xFF00CC66));
  monitorButton.setColour(juce::TextButton::textColourOffId,
                          juce::Colours::white.withAlpha(0.6f));
  monitorButton.setColour(juce::TextButton::textColourOnId,
                          juce::Colours::black);
  monitorButton.onClick = [this]() {
    bool on = monitorButton.getToggleState();
    monitorOn.store(on);
    monitorButton.setButtonText(on ? "MONITOR: ON" : "MONITOR: OFF");
  };
  addAndMakeVisible(monitorButton);

  // --- Settings Button ---
  settingsButton.setColour(juce::TextButton::buttonColourId,
                           juce::Colour(0xFF2A2A4A).withAlpha(0.8f));
  settingsButton.onClick = [this]() {
    if (deviceSelector == nullptr) {
      deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
          deviceManager, 1, 2, 2, 2, false, false, true, false);
      deviceSelector->setSize(400, 400);
      juce::DialogWindow::LaunchOptions options;
      options.content.setOwned(deviceSelector.release());
      options.dialogTitle = "AUDIO SETTINGS";
      options.dialogBackgroundColour = juce::Colours::black;
      options.escapeKeyTriggersCloseButton = true;
      options.resizable = false;
      options.useNativeTitleBar = true;
      options.launchAsync();
    }
  };

  // --- Middle Menu Panel (Mode & FX Tabs) ---
  addAndMakeVisible(middleMenuPanel);
  middleMenuPanel.setupModeControls(gainKnob, bpmKnob, monitorButton);
  middleMenuPanel.setupFxControls(fxXYPad, reverbSizeSlider, reverbSizeLabel);
  // Add settings button to mode controls indirectly or just add it here
  addAndMakeVisible(settingsButton);

  // --- Waveform Panel ---
  addAndMakeVisible(waveformPanel);
  waveformPanel.setBPM(120.0);

  // --- Riff History Panel ---
  addAndMakeVisible(riffHistoryPanel);
  riffHistoryPanel.setHistory(&riffHistory);
  riffHistoryPanel.onRiffSelected = [this](const Riff &riff) {
    if (riffEngine.getCurrentlyPlayingRiffId() == riff.id)
      return; // Already playing exactly this, ignore tap

    juce::Logger::writeToLog("Riff selected from history: " + riff.name);
    juce::AudioBuffer<float> composite;
    riff.getCompositeAudio(composite);
    riffEngine.playRiff(riff.id, composite, currentBpm.load(),
                        riff.sourceSampleRate, true);
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
    lastCaptureTime = now;

    // Immutable Stacking Logic: Every capture creates a NEW box in history.
    // If a riff is playing, we clone its state (layers) and add the new one.
    Riff newRiff;
    auto playingId = riffEngine.getCurrentlyPlayingRiffId();

    for (const auto &r : riffHistory.getHistory()) {
      if (r.id == playingId && !playingId.isNull()) {
        newRiff = r; // Deep copy previous state

        // Summing Logic: If we already have 8 layers, flatten them first.
        if (newRiff.layers >= 8) {
          juce::Logger::writeToLog(
              "Summing 8 layers into history to reset staircase.");
          newRiff.sumToSingleLayer();
        }
        break;
      }
    }

    newRiff.id = juce::Uuid(); // New identity for the new snapshot
    newRiff.bpm = bpm;
    newRiff.bars = bars;
    newRiff.sourceSampleRate = currentSampleRate;
    newRiff.name = "Riff " + juce::String(riffHistory.size() + 1);
    newRiff.captureTime = juce::Time::getCurrentTime();
    newRiff.source = "Microphone";

    // Capture the audio from the buffer
    juce::AudioBuffer<float> capturedAudio;
    retroBuffer.getAudioRegion(capturedAudio, numFrames);
    newRiff.merge(capturedAudio, bars);

    juce::Logger::writeToLog("Created History Snapshot: " + newRiff.name +
                             " (Layers: " + juce::String(newRiff.layers) + ")");

    // Add to history
    const auto &ref = riffHistory.addRiff(std::move(newRiff));
    juce::AudioBuffer<float> composite;
    ref.getCompositeAudio(composite);
    riffEngine.playRiff(ref.id, composite, bpm, ref.sourceSampleRate, true);

    riffHistoryPanel.repaint();
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

  // Prepare scratch buffers
  inputCopyBuffer.setSize(2, samplesPerBlockExpected);
  looperMixBuffer.setSize(2, samplesPerBlockExpected);
  riffOutputBuffer.setSize(2, samplesPerBlockExpected);

  // FX Crossfade (3ms)
  fxLevelSelector.reset(sampleRate, 0.003); // 3ms duration
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  auto *device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr) {
    bufferToFill.clearActiveBufferRegion();
    return;
  }

  auto *buffer = bufferToFill.buffer;
  const int numSamples = bufferToFill.numSamples;

  // 1. Process Input Gain
  float gain = gainLinear.load();
  buffer->applyGain(0, numSamples, gain);

  // 2. Snapshot current block to pre-allocated scratch buffer
  inputCopyBuffer.copyFrom(0, 0, *buffer, 0, 0, numSamples);
  if (buffer->getNumChannels() > 1)
    inputCopyBuffer.copyFrom(1, 0, *buffer, 1, 0, numSamples);

  // Calculate peak for level meter
  peakLevel.store(buffer->getMagnitude(0, numSamples));

  // 3. Prepare workspace buffers
  bufferToFill.clearActiveBufferRegion(); // Final output starts empty
  looperMixBuffer.copyFrom(0, 0, inputCopyBuffer, 0, 0, numSamples);
  if (looperMixBuffer.getWritePointer(1) != nullptr &&
      inputCopyBuffer.getNumChannels() > 1)
    looperMixBuffer.copyFrom(1, 0, inputCopyBuffer, 1, 0, numSamples);

  riffOutputBuffer.clear();

  // 4. Process Riffs ONCE into riffOutputBuffer
  if (isPlaying.load()) {
    riffEngine.processNextBlock(riffOutputBuffer, currentBpm.load(),
                                numSamples);
  }

  // Add Riffs to speaker output
  for (int ch = 0; ch < buffer->getNumChannels(); ++ch) {
    if (ch < riffOutputBuffer.getNumChannels()) {
      buffer->addFrom(ch, 0, riffOutputBuffer, ch, 0, numSamples);
    }
  }

  // Add Riffs to looper mix
  for (int ch = 0; ch < looperMixBuffer.getNumChannels(); ++ch) {
    if (ch < riffOutputBuffer.getNumChannels()) {
      looperMixBuffer.addFrom(ch, 0, riffOutputBuffer, ch, 0, numSamples);
    }
  }

  // 5. Monitor toggle (Speaker path only: add mic input back)
  if (monitorOn.load()) {
    for (int ch = 0; ch < buffer->getNumChannels(); ++ch) {
      buffer->addFrom(ch, 0, inputCopyBuffer, ch, 0, numSamples);
    }
  }

  // 6. FX Processing with 3ms Smooth Crossfade
  const bool active = fxXYPad.isMouseButtonDown();
  fxLevelSelector.setValue(active ? 1.0f : 0.0f);

  // Apply Delay Logic
  for (int i = 0; i < numSamples; ++i) {
    // Smoothed mix level (advance once per frame for stereo sync)
    const float currentMix = fxLevelSelector.getNextValue();

    for (int ch = 0; ch < buffer->getNumChannels(); ++ch) {
      float *outData = buffer->getWritePointer(ch);
      float *loopData = looperMixBuffer.getWritePointer(ch);
      float *dData = delayBuffer.getWritePointer(ch);
      const int dSize = delayBuffer.getNumSamples();
      const float dTime = delayTimeSec.load();
      const float dFeedback = delayFeedback.load();
      const int dSamples = static_cast<int>(currentSampleRate * dTime);

      const int readPos = (delayWritePos - dSamples + i + dSize) % dSize;
      const float dSample = dData[readPos];

      // Capture current dry signals BEFORE mixing FX (for feeding delay line)
      const float dryLoop = loopData[i];

      // Mixing for Speakers
      outData[i] = (outData[i] * (1.0f - currentMix)) + (dSample * currentMix);

      // Mixing for Looper
      loopData[i] = (dryLoop * (1.0f - currentMix)) + (dSample * currentMix);

      // Feed delay line from looper path (always has input + riffs)
      dData[(delayWritePos + i) % dSize] = dryLoop + (dSample * dFeedback);
    }
  }
  delayWritePos = (delayWritePos + numSamples) % delayBuffer.getNumSamples();

  // Reverb
  const float revMix = fxLevelSelector.getCurrentValue();
  reverbParams.roomSize = reverbRoomSize.load();
  reverbParams.wetLevel = revMix;
  reverbParams.dryLevel = 1.0f - revMix;
  reverb.setParameters(reverbParams);

  if (buffer->getNumChannels() >= 2) {
    reverb.processStereo(buffer->getWritePointer(0), buffer->getWritePointer(1),
                         numSamples);
    reverb.processStereo(looperMixBuffer.getWritePointer(0),
                         looperMixBuffer.getWritePointer(1), numSamples);
  } else {
    reverb.processMono(buffer->getWritePointer(0), numSamples);
    reverb.processMono(looperMixBuffer.getWritePointer(0), numSamples);
  }

  // 7. Push to retrospective buffer
  retroBuffer.pushBlock(looperMixBuffer);
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

  // Settings button position (top right, above meter maybe? or just next to it)
  settingsButton.setBounds(10, 10, 80, 25);
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
