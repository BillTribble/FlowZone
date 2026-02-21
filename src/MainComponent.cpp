#include "MainComponent.h"
#include <juce_core/juce_core.h>

//==============================================================================
MainComponent::MainComponent() {
  LOG_STARTUP();

  // --- Header UI ---
  addAndMakeVisible(bpmDisplay);
  addAndMakeVisible(playPauseButton);
  addAndMakeVisible(settingsButton);
  addAndMakeVisible(levelMeter);

  playPauseButton.setClickingTogglesState(true);
  playPauseButton.setToggleState(true, juce::dontSendNotification);
  playPauseButton.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xFF2A2A4A));
  playPauseButton.setColour(juce::TextButton::buttonOnColourId,
                            juce::Colour(0xFF00CC66));
  playPauseButton.onClick = [this]() {
    bool playing = playPauseButton.getToggleState();
    isPlaying.store(playing);
    playPauseButton.setButtonText(playing ? "PAUSE" : "PLAY");
  };

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

  // --- Main Framework & Tab Panels ---
  addAndMakeVisible(framework);
  addAndMakeVisible(waveformPanel);
  addAndMakeVisible(riffHistoryPanel);

  middleMenuPanel.onTabChanged = [this](MiddleMenuPanel::Tab tab) {
    updateLayoutForTab(tab);
  };
  middleMenuPanel.setupMicReverb();

  // --- Setup Tab-Specific Logic ---
  setupModeTabLogic();
  setupFXTabLogic();

  // --- Riff History Panel ---
  riffHistoryPanel.setHistory(&riffHistory);
  riffHistoryPanel.onRiffSelected = [this](const Riff &riff) {
    if (riffEngine.getCurrentlyPlayingRiffId() == riff.id)
      return;

    juce::Logger::writeToLog("Riff selected from history: " + riff.name);
    riffEngine.playRiff(riff, true);
  };
  riffHistoryPanel.isRiffPlaying = [this](const juce::Uuid &id) {
    return riffEngine.isRiffPlaying(id);
  };

  waveformPanel.setBPM(120.0);
  waveformPanel.onLoopTriggered = [this](int bars) {
    if (currentSampleRate <= 0.0)
      return;

    const double bpm = currentBpm.load();
    const double framesPerBar = currentSampleRate * (60.0 / bpm) * 4.0;

    // --- WYSIWYG Capture ---
    // Revert to visual sync directly to solve missing audio chunks.
    const int offsetSamples = 0;
    const int numFrames = static_cast<int>(framesPerBar * bars);

    Riff newRiff;
    juce::AudioBuffer<float> capturedAudio;
    auto playingId = riffEngine.getCurrentlyPlayingRiffId();

    for (const auto &r : riffHistory.getHistory()) {
      if (r.id == playingId && !playingId.isNull()) {
        newRiff = r;
        break;
      }
    }

    retroBuffer.getAudioRegion(capturedAudio, numFrames, offsetSamples);

    bool isFxTab = (activeTab == MiddleMenuPanel::Tab::FX);
    uint8_t mask = selectedLayers.load();

    if (isFxTab && mask != 0) {
      // FX Bounce: Selected layers (in mask) are consolidated into ONE wet
      // layer. commitFX handles replacing mask layers with capturedAudio.
      newRiff.commitFX(capturedAudio, mask);
      // Ensure we don't accidentally merge or sum again here
      selectedLayers.store(0xFF); // Reset to "all selected" for the new riff
    } else {
      // Normal Capture: Handle 8-layer limit correctly
      if (newRiff.layers >= 8) {
        // Record 9th -> then flatten all 9 into exactly ONE composite
        newRiff.merge(capturedAudio, bars);
        newRiff.sumToSingleLayer();
      } else {
        newRiff.merge(capturedAudio, bars);
      }
    }

    newRiff.id = juce::Uuid();
    newRiff.bpm = bpm;
    newRiff.bars = bars;
    newRiff.sourceSampleRate = currentSampleRate;
    newRiff.name = "Riff " + juce::String(riffHistory.size() + 1);
    newRiff.captureTime = juce::Time::getCurrentTime();
    newRiff.source = isFxTab ? "FX Bounce" : "Microphone";

    const auto &ref = riffHistory.addRiff(std::move(newRiff));
    riffEngine.playRiff(ref, true);
  };

  // --- File Logger ---
  auto logFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                     .getChildFile("FlowZone_Log.txt");
  juce::Logger::setCurrentLogger(
      new juce::FileLogger(logFile, "FlowZone Log started"));

  // Set default tab
  updateLayoutForTab(MiddleMenuPanel::Tab::Mode);

  setAudioChannels(2, 2);

  // Set default buffer size to 32 samples
  auto setup = deviceManager.getAudioDeviceSetup();
  if (setup.bufferSize != 32 && setup.bufferSize > 0) {
    setup.bufferSize = 32;
    deviceManager.setAudioDeviceSetup(setup, true);
  }

  startTimerHz(30);
  setSize(400, 750);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::setupModeTabLogic() {
  // Gain
  auto &gainSlider = modeBottom.getGainKnob().getSlider();
  gainSlider.setRange(-60.0, 12.0, 0.1);
  gainSlider.setValue(10.0);
  gainSlider.onValueChange = [this, &gainSlider]() {
    float dB = static_cast<float>(gainSlider.getValue());
    gainLinear.store(juce::Decibels::decibelsToGain(dB));
  };

  // Monitor
  auto &monBtn = modeBottom.getMonitorButton();
  monBtn.onClick = [this, &monBtn]() {
    bool on = monBtn.getToggleState();
    monitorOn.store(on);
    monBtn.setButtonText(on ? "MONITOR: ON" : "MONITOR: OFF");
    LOG_ACTION("Mic", on ? "Monitor ON" : "Monitor OFF");
  };

  auto &bypassBtn = modeBottom.getBypassButton();
  bypassBtn.onClick = [this, &bypassBtn]() {
    bool bypassed = bypassBtn.getToggleState();
    micReverbBypassed.store(bypassed);
    bypassBtn.setButtonText(bypassed ? "BYPASS: ON" : "BYPASS: OFF");
    LOG_ACTION("Mic", bypassed ? "Reverb Bypass ON" : "Reverb Bypass OFF");
  };

  // Reverb
  auto &sizeSlider = modeBottom.getReverbSizeSlider();
  sizeSlider.setRange(0.0, 1.0, 0.01);
  sizeSlider.setValue(0.5);
  sizeSlider.onValueChange = [this, &sizeSlider]() {
    micReverbRoomSize.store((float)sizeSlider.getValue());
  };

  auto &mixSlider = modeBottom.getReverbMixSlider();
  mixSlider.setRange(0.0, 1.0, 0.01);
  mixSlider.setValue(0.0);
  mixSlider.onValueChange = [this, &mixSlider]() {
    micReverbWetLevel.store((float)mixSlider.getValue());
  };
}

void MainComponent::setupFXTabLogic() {
  fxTop.getFXGrid().onSelectionChanged = [this](int idx) {
    LOG_ACTION("FX", "Mode Changed to: " + juce::String(idx));
  };

  fxBottom.getLayerGrid().onSelectionChanged = [this](uint8_t mask) {
    selectedLayers.store(mask);
  };
  selectedLayers.store(0xFF);

  auto &pad = fxBottom.getXYPad();
  pad.onXYChange = [this](float x, float y) {
    fxEngine.setDelayParams(x * 2.0f, y * 0.8f);
    fxEngine.setReverbParams(x, y * 0.5f);
  };

  pad.onRelease = [this, &pad]() { LOG_ACTION("FX", "XY Pad Released"); };
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  currentSampleRate = sampleRate;
  retroBuffer.prepare(sampleRate, 60);
  riffEngine.prepare(sampleRate, samplesPerBlockExpected);
  waveformPanel.setSampleRate(sampleRate);
  micReverb.setSampleRate(sampleRate);
  fxEngine.prepare({sampleRate, (juce::uint32)samplesPerBlockExpected, 2});

  inputCopyBuffer.setSize(2, samplesPerBlockExpected);
  looperMixBuffer.setSize(2, samplesPerBlockExpected);
  riffOutputBuffer.setSize(2, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  auto *buffer = bufferToFill.buffer;
  const int numSamples = bufferToFill.numSamples;
  const int numChannels = buffer->getNumChannels();
  const double bpm = currentBpm.load();

  // --- 1. Master Clock Transport ---
  double startPpq = 0.0;
  if (isPlaying.load()) {
    double beatsPerSecond = bpm / 60.0;
    double beatsPerSample = beatsPerSecond / currentSampleRate;
    startPpq = playbackPosition.load();
    playbackPosition.store(startPpq + (numSamples * beatsPerSample));

    double maxBeats = 32.0 * 4.0;
    if (playbackPosition.load() >= maxBeats)
      playbackPosition.store(playbackPosition.load() - maxBeats);
  }

  float gain = gainLinear.load();
  buffer->applyGain(0, numSamples, gain);

  // --- 1. Mono to Stereo Fix ---
  inputCopyBuffer.setSize(2, numSamples, false, true, true);
  if (numChannels > 0) {
    // Always treat as L->L+R mapping for mono consistency
    inputCopyBuffer.copyFrom(0, 0, *buffer, 0, 0, numSamples);
    inputCopyBuffer.copyFrom(1, 0, *buffer, 0, 0, numSamples);
  } else {
    inputCopyBuffer.clear();
  }

  // --- 4. Mic Reverb Routing (Input-Only) ---
  if (!micReverbBypassed.load()) {
    micReverbParams.roomSize = micReverbRoomSize.load();
    micReverbParams.wetLevel = micReverbWetLevel.load();
    micReverbParams.dryLevel = 1.0f - micReverbWetLevel.load();
    micReverb.setParameters(micReverbParams);

    if (inputCopyBuffer.getNumChannels() >= 2)
      micReverb.processStereo(inputCopyBuffer.getWritePointer(0),
                              inputCopyBuffer.getWritePointer(1), numSamples);
  }

  buffer->clear();

  riffOutputBuffer.clear();
  looperMixBuffer.clear();
  uint8_t mask = selectedLayers.load();

  if (isPlaying.load()) {
    riffEngine.processNextBlock(riffOutputBuffer, looperMixBuffer, bpm,
                                numSamples, startPpq, mask);
  }

  // --- 2. FX Mode Buffer Routing & Display Fix ---
  bool isFxTab = (activeTab == MiddleMenuPanel::Tab::FX);
  bool isPadDown = fxBottom.getXYPad().isMouseButtonDown();

  if (isFxTab) {
    if (mask != 0) {
      if (isPadDown) {
        juce::dsp::AudioBlock<float> block(looperMixBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        fxEngine.process(context);
      }
      // FX Mode: Looper ALWAYS sees the selected layers (wet if pad down, dry
      // if pad up)
      retroBuffer.pushBlock(looperMixBuffer);
    } else {
      // No layers selected - just keep looper fed with Mic
      retroBuffer.pushBlock(inputCopyBuffer);
    }
  } else {
    // Normal Mode: Looper sees the DRY Mic signal
    retroBuffer.pushBlock(inputCopyBuffer);

    if (mask != 0) {
      for (int ch = 0; ch < riffOutputBuffer.getNumChannels(); ++ch) {
        if (ch < looperMixBuffer.getNumChannels())
          riffOutputBuffer.addFrom(ch, 0, looperMixBuffer, ch, 0, numSamples);
      }
      looperMixBuffer.clear();
    }
  }

  // --- 5. Final Hardware Mix (Monitoring) ---
  const uint8_t selectionMask = mask;
  const uint8_t remainderMask = (~selectionMask) & 0xFF;

  for (int ch = 0; ch < numChannels; ++ch) {
    // A. Unselected Dry Layers: Always audible in dry riff output buffer if
    // routed correctly. riffEngine.processNextBlock already handles the
    // mask-based split. Dry component (unselected)
    if (ch < riffOutputBuffer.getNumChannels())
      buffer->addFrom(ch, 0, riffOutputBuffer.getReadPointer(ch), numSamples);

    // B. Selected Layers & Mic Monitor
    if (isFxTab) {
      // FX Mode: Selected layers (looperMixBuffer) are always audible.
      // Mic input is suppressed.
      if (mask != 0 && ch < looperMixBuffer.getNumChannels()) {
        buffer->addFrom(ch, 0, looperMixBuffer.getReadPointer(ch), numSamples);
      }
    } else {
      // Normal Mode: Selected layers are already in riffOutputBuffer.
      // We only need to add the live Mic signal if Monitor is ON.
      if (monitorOn.load() && ch < inputCopyBuffer.getNumChannels()) {
        buffer->addFrom(ch, 0, inputCopyBuffer.getReadPointer(ch), numSamples);
      }
    }

    if (ch == 0) {
      float peak = buffer->getMagnitude(ch, 0, numSamples);
      if (peak > peakLevel.load())
        peakLevel.store(peak);
    }
  }
}

void MainComponent::releaseResources() {}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF0F0F23));
  juce::ColourGradient bgGradient(juce::Colour(0xFF16162B), 0, 0,
                                  juce::Colour(0xFF0A0A1A), 0,
                                  (float)getHeight(), false);
  g.setGradientFill(bgGradient);
  g.fillRect(getLocalBounds());
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.fillRect(20, 50, getWidth() - 40, 1);
}

void MainComponent::resized() {
  auto area = getLocalBounds();

  // 1. Header (Static layout for now)
  auto headerArea = area.removeFromTop(40);
  settingsButton.setBounds(headerArea.removeFromLeft(100).reduced(5, 5));
  playPauseButton.setBounds(headerArea.removeFromRight(80).reduced(2, 5));
  bpmDisplay.setBounds(headerArea.removeFromRight(100));
  levelMeter.setHorizontal(true);
  levelMeter.setBounds(headerArea.reduced(20, 12));

  // 2. Riff History (Fixed bottom)
  auto historyArea = area.removeFromBottom(85);
  riffHistoryPanel.setBounds(historyArea);

  // 3. Waveform Timeline (Fixed above history)
  auto waveformArea = area.removeFromBottom(120);
  waveformPanel.setBounds(waveformArea);

  // 4. Sandwich Framework (Top Content + Tabs + Performance)
  framework.setBounds(area);
}

void MainComponent::updateLayoutForTab(MiddleMenuPanel::Tab tab) {
  activeTab = tab;
  LOG_ACTION("UI", "Tab Switched to: " + juce::String((int)tab));

  if (tab == MiddleMenuPanel::Tab::Mode) {
    framework.setTopComponent(&modeTop);
    framework.setBottomComponent(&modeBottom);
  } else if (tab == MiddleMenuPanel::Tab::FX) {
    framework.setTopComponent(&fxTop);
    framework.setBottomComponent(&fxBottom);
  } else if (tab == MiddleMenuPanel::Tab::Mixer) {
    framework.setTopComponent(&mixerTop);
    framework.setBottomComponent(&mixerBottom);
  }
}

void MainComponent::timerCallback() {
  levelMeter.setLevel(peakLevel.load());
  peakLevel.store(0.0f); // Reset peak after reading

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

  if (activeTab == MiddleMenuPanel::Tab::FX) {
    auto playingId = riffEngine.getCurrentlyPlayingRiffId();
    uint8_t enabledMask = 0;

    if (!playingId.isNull()) {
      for (const auto &riff : riffHistory.getHistory()) {
        if (riff.id == playingId) {
          int count = riff.layers;
          for (int i = 0; i < count && i < 8; ++i)
            enabledMask |= (1 << i);
          break;
        }
      }
    }
    fxBottom.getLayerGrid().setEnabledMask(enabledMask);
  }
}

void MainComponent::updateBpm(double newBpm) {
  newBpm = std::clamp(newBpm, 40.0, 240.0);
  currentBpm.store(newBpm);
  waveformPanel.setBPM(newBpm);
  bpmDisplay.repaint();
}
