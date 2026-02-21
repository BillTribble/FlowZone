#include "MainComponent.h"
#include <juce_core/juce_core.h>

//==============================================================================
MainComponent::MainComponent() {
  LOG_STARTUP();
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

  // --- BPM Display (Header) ---
  addAndMakeVisible(bpmDisplay);

  // bpmDisplay initialization is handled above

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
    LOG_ACTION("Mic", on ? "Monitor ON" : "Monitor OFF");
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

  // --- Mic Reverb UI (Mode Tab) ---
  micReverbSizeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  micReverbSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  micReverbSizeSlider.setRange(0.0, 1.0, 0.01);
  micReverbSizeSlider.setValue(0.5);
  micReverbSizeSlider.onValueChange = [this]() {
    float val = (float)micReverbSizeSlider.getValue();
    micReverbRoomSize.store(val);
    LOG_ACTION("Mic", "Reverb Size: " + juce::String(val, 2));
  };

  micReverbMixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  micReverbMixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  micReverbMixSlider.setRange(0.0, 1.0, 0.01);
  micReverbMixSlider.setValue(0.0); // Default dry
  micReverbMixSlider.onValueChange = [this]() {
    float val = (float)micReverbMixSlider.getValue();
    micReverbWetLevel.store(val);
    LOG_ACTION("Mic", "Reverb Mix: " + juce::String(val, 2));
  };

  // --- Initial Layout Configuration ---
  middleMenuPanel.onTabChanged = [this](MiddleMenuPanel::Tab tab) {
    updateLayoutForTab(tab);
  };

  layerGrid.onSelectionChanged = [this](uint8_t mask) {
    selectedLayers.store(mask);
  };

  activeXYPad.onXYChange = [this](float x, float y) {
    fxEngine.setDelayParams(x * 2.0f, y * 0.8f); // X=Time, Y=Feedback
    fxEngine.setReverbParams(x, y * 0.5f);       // X=RoomSize, Y=Mix
  };

  activeXYPad.onRelease = [this]() {
    LOG_ACTION("FX", "XY Pad Released - Committing FX");

    auto playingId = riffEngine.getCurrentlyPlayingRiffId();
    if (playingId.isNull())
      return;

    // Find the riff in history
    Riff *targetRiff = nullptr;
    for (auto &r : riffHistory.getHistoryRW()) {
      if (r.id == playingId) {
        targetRiff = &r;
        break;
      }
    }

    if (targetRiff == nullptr)
      return;

    uint8_t mask = selectedLayers.load();
    if (mask == 0)
      return;

    // 1. Get the isolated audio for selected layers
    juce::AudioBuffer<float> wetSum;
    targetRiff->getSelectedLayerComposite(wetSum, mask);

    if (wetSum.getNumSamples() == 0)
      return;

    // 2. Process it through a temporary engine to ensure clean/full baking
    // We use the current params but a fresh state.
    StandaloneFXEngine bakeEngine;
    bakeEngine.prepare(
        {currentSampleRate, (juce::uint32)wetSum.getNumSamples(), 2});

    // Copy current params from real-time engine if we had getters,
    // but we'll just use the XYPad coords for now as they are the source of
    // truth.
    float x = activeXYPad.getXValue();
    float y = activeXYPad.getYValue();
    bakeEngine.setDelayParams(x * 2.0f, y * 0.8f);
    bakeEngine.setReverbParams(x, y * 0.5f);

    juce::dsp::AudioBlock<float> block(wetSum);
    juce::dsp::ProcessContextReplacing<float> context(block);
    bakeEngine.process(context);

    // 3. Commit back to riff
    targetRiff->commitFX(wetSum, mask);

    // 4. Update playback engine with new riff data
    // (Important: Since RiffPlaybackEngine stores its own copies, we
    // re-trigger)
    riffEngine.playRiff(*targetRiff, true);

    juce::Logger::writeToLog("FX Committed to Riff: " + targetRiff->name);
    LOG_ACTION("FX", "Destructive Commit Done - " + targetRiff->name);

    // Reset selection mask after commit to avoid accidental double-processing
    selectedLayers.store(0);
    // TODO: Update layerGrid UI state if needed
  };

  // Set default tab
  updateLayoutForTab(MiddleMenuPanel::Tab::Mode);

  addAndMakeVisible(topContentPanel);
  addAndMakeVisible(bottomPerformancePanel);
  addAndMakeVisible(middleMenuPanel);
  middleMenuPanel.setupModeControls(gainKnob, monitorButton);
  middleMenuPanel.setupMicReverb(micReverbSizeSlider, micReverbMixSlider);
  middleMenuPanel.setupFxControls();
  addAndMakeVisible(settingsButton);

  instrumentModeGrid = std::make_unique<SelectionGrid>(
      2, 4,
      juce::StringArray{"Keys", "Drums", "Synth", "Voice", "Bass", "Guitar",
                        "Brass", "Pad"});
  instrumentModeGrid->onSelectionChanged = [this](int idx) {
    LOG_ACTION("Instrument", "Mode Changed to: " + juce::String(idx));
  };

  soundPresetGrid = std::make_unique<SelectionGrid>(
      3, 4,
      juce::StringArray{"Natural", "Space", "Lo-Fi", "Acid", "Clean", "Crunch",
                        "Lead", "Atmosphere", "Pulse", "Twitch", "Bells",
                        "Drone"});
  soundPresetGrid->onSelectionChanged = [this](int idx) {
    LOG_ACTION("Sound", "Preset Changed to: " + juce::String(idx));
  };

  // --- File Logger for Troubleshooting ---
  auto logFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                     .getChildFile("FlowZone_Log.txt");
  juce::Logger::setCurrentLogger(
      new juce::FileLogger(logFile, "FlowZone Log started"));
  juce::Logger::writeToLog("Logging to: " + logFile.getFullPathName());

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
    riffEngine.playRiff(riff, true);
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
    // This closing brace was misplaced, it should be after the loop.
    // The original code had it here, which meant newRiff.id was set outside the
    // conditional. The instruction implies removing it, but it's syntactically
    // incorrect to remove it without replacing it. Assuming the intent was to
    // remove the extra brace and keep the logic within the loop. If the intent
    // was to remove the entire `if (lastRiff)` block, the instruction was
    // unclear. Given the context of "Fix API mismatches and call sites", this
    // looks like a structural fix. The original code had an extra `}` here,
    // which was likely a copy-paste error. Removing it makes the code
    // syntactically correct and aligns with the `newRiff.id = juce::Uuid();`
    // line.

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
    riffEngine.playRiff(ref, true);
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

  micReverb.setSampleRate(sampleRate);
  fxEngine.prepare({sampleRate, (juce::uint32)samplesPerBlockExpected, 2});

  // Prepare scratch buffers
  inputCopyBuffer.setSize(2, samplesPerBlockExpected);
  looperMixBuffer.setSize(2, samplesPerBlockExpected);
  riffOutputBuffer.setSize(2, samplesPerBlockExpected);
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
  const int numChannels = buffer->getNumChannels();
  const double bpm = currentBpm.load();

  // 1. Process Input Gain
  float gain = gainLinear.load();
  buffer->applyGain(0, numSamples, gain);

  // 2. Capture Dry Input for Retrospective Buffer
  // We do this BEFORE monitoring/adding riffs so it's always "just the
  // performance"
  inputCopyBuffer.setSize(numChannels, numSamples, false, true, true);
  inputCopyBuffer.copyFrom(0, 0, *buffer, 0, 0, numSamples);
  if (numChannels > 1)
    inputCopyBuffer.copyFrom(1, 0, *buffer, 1, 0, numSamples);

  // 3. Clear output if monitor is off, otherwise monitor is already in buffer
  if (!monitorOn.load()) {
    buffer->clear();
  }

  // --- V9 Selective Riff Summing ---
  // 1. Get Selected Layers into scratch buffer (Isolator)
  riffOutputBuffer.clear(); // Using this as scratch for Dry sum first
  looperMixBuffer.clear();

  uint8_t mask = selectedLayers.load();

  // Dry Sum (everything NOT in mask)
  if (isPlaying.load()) {
    riffEngine.processNextBlock(riffOutputBuffer, bpm, numSamples, ~mask);
  }

  // Wet Sum (everything IN mask)
  if (isPlaying.load()) {
    riffEngine.processNextBlock(looperMixBuffer, bpm, numSamples, mask);
  }

  // 2. Process Wet sum through FX Engine
  if (mask != 0) {
    juce::dsp::AudioBlock<float> block(looperMixBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    fxEngine.process(context);
  }

  // 3. Blend result back into main output
  for (int ch = 0; ch < numChannels; ++ch) {
    // Add Dry Riffs
    if (ch < riffOutputBuffer.getNumChannels()) {
      buffer->addFrom(ch, 0, riffOutputBuffer.getReadPointer(ch), numSamples);
    }

    // Add Wet Riffs (Selective FX result)
    if (ch < looperMixBuffer.getNumChannels()) {
      buffer->addFrom(ch, 0, looperMixBuffer.getReadPointer(ch), numSamples);
    }

    // Re-apply Level Meter monitoring to the final sum
    if (ch == 0) {
      float peak = buffer->getMagnitude(ch, 0, numSamples);
      float currentPeak = peakLevel.load();
      if (peak > currentPeak)
        peakLevel.store(peak);

      LOG_AUDIO_STATS(peak, peak > 1.0f);
    }
  }

  // --- Mic Reverb (Vocal Polish) ---
  micReverbParams.roomSize = micReverbRoomSize.load();
  micReverbParams.wetLevel = micReverbWetLevel.load();
  micReverbParams.dryLevel = 1.0f - micReverbWetLevel.load();
  micReverb.setParameters(micReverbParams);

  if (buffer->getNumChannels() >= 2) {
    micReverb.processStereo(buffer->getWritePointer(0),
                            buffer->getWritePointer(1), numSamples);
  } else {
    micReverb.processMono(buffer->getWritePointer(0), numSamples);
  }

  // 7. Push to retrospective buffer
  // We push the looperMixBuffer which should contain the CLEAN input
  // (inputCopyBuffer)
  looperMixBuffer.copyFrom(0, 0, inputCopyBuffer, 0, 0, numSamples);
  if (numChannels > 1)
    looperMixBuffer.copyFrom(1, 0, inputCopyBuffer, 1, 0, numSamples);

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
  auto area = getLocalBounds();

  // 1. Header Area (Fixed 40px)
  auto headerArea = area.removeFromTop(40);
  settingsButton.setBounds(headerArea.removeFromLeft(100).reduced(5, 5));
  playPauseButton.setBounds(headerArea.removeFromRight(80).reduced(2, 5));
  bpmDisplay.setBounds(headerArea.removeFromRight(100));
  levelMeter.setHorizontal(true);
  levelMeter.setBounds(headerArea.reduced(20, 12));

  // 2. Riff History (Fixed 50px at the very bottom)
  auto historyArea = area.removeFromBottom(50);
  riffHistoryPanel.setBounds(historyArea);

  // 3. Bottom Performance Panel (Fixed 240px above history)
  auto performanceArea = area.removeFromBottom(240);
  bottomPerformancePanel.setBounds(performanceArea);

  // 4. Waveform Timeline (Fixed 120px above performance)
  auto waveformArea = area.removeFromBottom(120);
  waveformPanel.setBounds(waveformArea);

  // 5. Middle Menu Tab Bar (Fixed 40px above waveform)
  auto tabArea = area.removeFromBottom(40);
  middleMenuPanel.setBounds(tabArea);

  // 6. Top Content Panel (Remaining middle space, ~260px)
  topContentPanel.setBounds(area);

  if (instrumentModeGrid) {
    instrumentModeGrid->setBounds(topContentPanel.getLocalBounds());
  }
  if (soundPresetGrid) {
    soundPresetGrid->setBounds(topContentPanel.getLocalBounds());
  }
  layerGrid.setBounds(topContentPanel.getLocalBounds());
  activeXYPad.setBounds(bottomPerformancePanel.getLocalBounds());
}

void MainComponent::updateLayoutForTab(MiddleMenuPanel::Tab tab) {
  activeTab = tab;

  LOG_ACTION("UI", "Tab Switched to: " + juce::String((int)tab));

  // 1. Reset Visibility
  gainKnob.setVisible(false);
  monitorButton.setVisible(false);
  micReverbSizeSlider.setVisible(false);
  micReverbMixSlider.setVisible(false);
  layerGrid.setVisible(false);
  activeXYPad.setVisible(false);

  if (instrumentModeGrid)
    instrumentModeGrid->setVisible(false);
  if (soundPresetGrid)
    soundPresetGrid->setVisible(false);

  // 2. Configure based on tab
  if (tab == MiddleMenuPanel::Tab::Mode) {
    // Mode tab (Instrument)
    if (instrumentModeGrid) {
      topContentPanel.addAndMakeVisible(*instrumentModeGrid);
      instrumentModeGrid->setVisible(true);
    }

    bottomPerformancePanel.addAndMakeVisible(gainKnob);
    bottomPerformancePanel.addAndMakeVisible(monitorButton);
    bottomPerformancePanel.addAndMakeVisible(micReverbSizeSlider);
    bottomPerformancePanel.addAndMakeVisible(micReverbMixSlider);

    gainKnob.setVisible(true);
    monitorButton.setVisible(true);
    micReverbSizeSlider.setVisible(true);
    micReverbMixSlider.setVisible(true);
  } else if (tab == MiddleMenuPanel::Tab::FX) {
    // FX tab
    topContentPanel.addAndMakeVisible(layerGrid);
    layerGrid.setVisible(true);

    bottomPerformancePanel.addAndMakeVisible(activeXYPad);
    activeXYPad.setVisible(true);
  } else if (tab == MiddleMenuPanel::Tab::Sound) {
    // Sound tab (Presets)
    if (soundPresetGrid) {
      topContentPanel.addAndMakeVisible(*soundPresetGrid);
      soundPresetGrid->setVisible(true);
    }
  }

  resized();
}

void MainComponent::timerCallback() {
  // Read peak from audio thread (atomic) and update the meter
  levelMeter.setLevel(peakLevel.load());

  // Feed waveform panel sections: 8, 4, 2, 1 bars at current BPM
  if (currentSampleRate > 0.0) {
    const double bpm = currentBpm.load();
    const int panelW = std::max(waveformPanel.getWidth(), 1);
    const int framesPerBar =
        static_cast<int>(currentSampleRate * (60.0 / bpm) * 4.0);

    const int sectionW = std::max(panelW / 4, 1);
    const int bars[] = {8, 4, 2, 1};

    for (int i = 0; i < 4; ++i) {
      const int numFrames = static_cast<int>(framesPerBar * bars[i]);
      auto sectionData = retroBuffer.getWaveformData(numFrames, sectionW);
      waveformPanel.setSectionData(i, sectionData);
    }
  }
}

void MainComponent::updateBpm(double newBpm) {
  newBpm = std::clamp(newBpm, 40.0, 240.0);
  currentBpm.store(newBpm);
  waveformPanel.setBPM(newBpm);
  bpmDisplay.repaint();
}
