#include "FlowEngine.h"
#include "FileLogger.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace flowzone {

FlowEngine::FlowEngine() : juce::Thread("FlowEngine"), juce::Timer() {
  for (int i = 0; i < 8; ++i) {
    slots.push_back(std::make_unique<Slot>(i));
  }
}

FlowEngine::~FlowEngine() { stopThread(2000); }

void FlowEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;

  transport.prepareToPlay(sampleRate, samplesPerBlock);
  retroBuffer.prepare(sampleRate, 60); // 60 seconds of history
  featureExtractor.prepare(sampleRate, samplesPerBlock);

  // Prepare audio engines
  drumEngine.prepare(sampleRate, samplesPerBlock);
  synthEngine.prepare(sampleRate, samplesPerBlock);
  micProcessor.prepare(sampleRate, samplesPerBlock);

  for (auto &slot : slots) {
    slot->prepareToPlay(sampleRate, samplesPerBlock);
  }

  // Pre-allocate buffers for audio thread
  engineBuffer.setSize(2, samplesPerBlock);
  retroCaptureBuffer.setSize(2, samplesPerBlock);

  // Start UI broadcasting timer (60Hz = 16.6ms)
  startTimerHz(60);

  // Load last session on startup
  auto appData =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone");
  if (!appData.exists())
    appData.createDirectory();

  auto sessionFile = appData.getChildFile("last_session.flow");
  if (sessionFile.existsAsFile()) {
    AppState loadedState;
    if (sessionManager.loadSession(sessionFile, loadedState).wasOk()) {
      sessionManager.setState(loadedState);
      juce::Logger::writeToLog("Loaded existing session state with " +
                               juce::String(loadedState.sessions.size()) +
                               " jams.");
    }
  } else {
    // First run: create a default session
    createNewJam();
  }
}

void FlowEngine::updatePeakLevel(const juce::AudioBuffer<float> &buffer) {
  float peak = 0.0f;
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
    peak = std::max(peak, channelPeak);
  }

  // Use a decay similar to MicProcessor for smooth UI metering
  float currentPeak = retroBufferPeakLevel.load();
  if (peak > currentPeak) {
    retroBufferPeakLevel.store(peak);
  } else {
    // Decay slowly (~150ms to drop significantly at 44.1kHz/512)
    retroBufferPeakLevel.store(currentPeak * 0.95f);
  }
}

void FlowEngine::processBlock(juce::AudioBuffer<float> &buffer,
                              juce::MidiBuffer &midiMessages) {
  static int audioLogCounter = 0;
  bool shouldLog =
      (++audioLogCounter >=
       (int)(currentSampleRate / buffer.getNumSamples())); // ~1/sec
  if (shouldLog)
    audioLogCounter = 0;

  // Log input buffer peak and per-channel peaks for diagnostics
  if (shouldLog) {
    float inputPeak = 0.0f;
    std::string channelPeaks = "";
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float p = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
      inputPeak = std::max(inputPeak, p);
      channelPeaks += " ch" + std::to_string(ch) + "=" + std::to_string(p);
    }
    FileLogger::instance().log(
        FileLogger::Category::AudioFlow,
        "INPUT peak=" + std::to_string(inputPeak) +
            " numChannels=" + std::to_string(buffer.getNumChannels()) +
            " peaks: " + channelPeaks);
  }

  // Process incoming commands from Message Thread
  processCommands();

  // Handle Transport
  transport.processBlock(buffer, midiMessages);

  // Merge incoming MIDI with triggered notes
  juce::MidiBuffer combinedMidi(midiMessages);
  combinedMidi.addEvents(activeMidi, 0, buffer.getNumSamples(), 0);
  activeMidi.clear(); // Clear for next block

  // Process audio engines based on active mode
  auto state = sessionManager.getCurrentState();

  if (shouldLog) {
    FileLogger::instance().log(
        FileLogger::Category::AudioFlow,
        "MODE category=" + state.activeMode.category.toStdString() +
            " preset=" + state.activeMode.presetId.toStdString());
  }

  // Clear pre-allocated buffers
  engineBuffer.clear();
  retroCaptureBuffer.clear();

  if (state.activeMode.category == "mic") {
    // Mic mode: process input through MicProcessor
    // MicProcessor writes processed audio to engineBuffer for retrospective
    // capture
    micProcessor.process(buffer, engineBuffer);

    // Capture processed mic input-sized block to retro capture buffer
    for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                     ch < engineBuffer.getNumChannels();
         ++ch) {
      retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0,
                                  buffer.getNumSamples());
    }

    // Only add to main output if monitoring is enabled
    if (state.mic.monitorInput || state.mic.monitorUntilLooped) {
      // Add monitored signal to output
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.addFrom(ch, 0, engineBuffer, ch, 0, buffer.getNumSamples());
      }
    }
    // BUG FIX: Removed unconditional buffer.addFrom that was outside this if
    // block

  } else if (state.activeMode.category == "drums") {
    drumEngine.process(engineBuffer, combinedMidi);
    for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                     ch < engineBuffer.getNumChannels();
         ++ch) {
      retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0,
                                  buffer.getNumSamples());
    }
  } else if (state.activeMode.category == "notes" ||
             state.activeMode.category == "bass") {
    synthEngine.process(engineBuffer, combinedMidi);
    for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                     ch < engineBuffer.getNumChannels();
         ++ch) {
      retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0,
                                  buffer.getNumSamples());
    }
  }

  // ONLY add engine output to buffer for NON-MIC modes here.
  // Mic mode is already handled above with monitoring logic.
  if (state.activeMode.category != "mic") {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      buffer.addFrom(ch, 0, engineBuffer, ch, 0, buffer.getNumSamples());
    }
  }

  // Log engine output and retro capture peaks
  if (shouldLog) {
    float engPeak = 0.0f;
    float retroPeak = 0.0f;
    for (int ch = 0; ch < engineBuffer.getNumChannels(); ++ch)
      engPeak = std::max(
          engPeak, engineBuffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    for (int ch = 0; ch < retroCaptureBuffer.getNumChannels(); ++ch)
      retroPeak = std::max(retroPeak, retroCaptureBuffer.getMagnitude(
                                          ch, 0, buffer.getNumSamples()));
    FileLogger::instance().log(
        FileLogger::Category::AudioFlow,
        "ENGINE peak=" + std::to_string(engPeak) +
            " RETRO_CAPTURE peak=" + std::to_string(retroPeak));
  }

  // Update peak level for UI (atomic store)
  updatePeakLevel(retroCaptureBuffer);

  // Capture to retro buffer and feature extractor
  retroBuffer.pushBlock(retroCaptureBuffer);
  featureExtractor.pushAudioBlock(retroCaptureBuffer);

  if (shouldLog) {
    FileLogger::instance().log(FileLogger::Category::AudioFlow,
                               "RETRO_BUFFER_PEAK atomic=" +
                                   std::to_string(retroBufferPeakLevel.load()));
  }

  // Sum slots into buffer
  for (auto &slot : slots) {
    slot->processBlock(buffer, buffer.getNumSamples());
  }

  // Check for sync merge arrival
  performMergeSync();
}

void FlowEngine::processCommands() {
  juce::String commandStr;
  bool stateChanged = false;
  // Drain the queue
  while (commandQueue.pop(commandStr)) {
    dispatcher.dispatch(commandStr, *this);
  }
}

void FlowEngine::broadcastState() {
  static int stateLogCounter = 0;
  bool shouldLog = (++stateLogCounter >= 60); // ~1/sec at 60Hz
  if (shouldLog)
    stateLogCounter = 0;

  // This is called FROM THE MESSAGE THREAD via timerCallback
  auto state = sessionManager.getCurrentState();

  // Sync Transport
  state.transport.isPlaying = transport.isPlaying();
  state.transport.bpm = transport.getBpm();
  state.transport.metronomeEnabled = transport.isMetronomeEnabled();
  state.transport.loopLengthBars = transport.getLoopLengthBars();
  state.transport.barPhase = transport.getBarPhase();

  // Sync Mic Input Level
  state.mic.inputLevel = micProcessor.getPeakLevel();

  // Sync Looper Input Level (peak level tracked in audio thread)
  state.looper.inputLevel = retroBufferPeakLevel.load();

  // Sync Looper Waveform Data (O(N) operation - now safe on message thread)
  state.looper.waveformData = retroBuffer.getWaveformData(256);

  if (shouldLog) {
    // Check if waveform has any non-zero data
    bool hasWaveform = false;
    for (float v : state.looper.waveformData) {
      if (v > 0.001f) {
        hasWaveform = true;
        break;
      }
    }
    FileLogger::instance().log(
        FileLogger::Category::StateBroadcast,
        "STATE mic.inputLevel=" + std::to_string(state.mic.inputLevel) +
            " looper.inputLevel=" + std::to_string(state.looper.inputLevel) +
            " waveformHasData=" + std::string(hasWaveform ? "YES" : "NO") +
            " mode=" + state.activeMode.category.toStdString());
  }

  // Use patch-based update
  broadcaster.broadcastStateUpdate(state);
}

void FlowEngine::setLoopLength(int bars) {
  // Check for empty slot
  int targetSlot = -1;
  for (int i = 0; i < (int)slots.size(); ++i) {
    if (!slots[i]->isFull()) {
      targetSlot = i;
      break;
    }
  }

  if (targetSlot == -1) {
    // All 8 slots full! Trigger Auto-Merge
    nextCaptureBars = bars;
    triggerAutoMerge();
  } else {
    // Capture into targetSlot
    juce::AudioBuffer<float> captured;
    int samples = (int)(bars * transport.getSamplesPerBar());
    retroBuffer.getPastAudio(0, samples, captured);
    slots[targetSlot]->setAudioData(captured);
    slots[targetSlot]->getState().loopLengthBars = bars;
  }
}

void FlowEngine::triggerAutoMerge() {
  if (mergePending.load())
    return;

  mergePending.store(true);
  startThread();
}

void FlowEngine::run() {
  // Background thread: Sum slots 1-8
  // In V1, we just simulate the merge process for now
  // and prepare the data for the atomic swap in the audio thread.

  // Real implementation would sum audio here.
  juce::Thread::sleep(50); // Simulate work
}

void FlowEngine::performMergeSync() {
  if (!mergePending.load())
    return;

  // Audio thread transition:
  // 1. Sum Slots 1-8 into Slot 1
  // 2. Clear 2-8
  // 3. Trigger new capture into Slot 2

  juce::AudioBuffer<float> merger;
  int maxSamples = 0;
  for (auto &s : slots)
    maxSamples = std::max(
        maxSamples, (int)(s->getLengthInBars() * transport.getSamplesPerBar()));

  merger.setSize(2, maxSamples);
  merger.clear();

  for (auto &s : slots) {
    // Basic sum in audio thread for V1/Proof-of-concept
    // (Actual background sum is safer for CPU, but this works for testing)
    s->processBlock(merger, maxSamples);
  }

  slots[0]->setAudioData(merger);
  slots[0]->getState().loopLengthBars =
      (int)(maxSamples / transport.getSamplesPerBar());
  slots[0]->getState().presetId = "auto_merge";

  for (int i = 1; i < 8; ++i) {
    slots[i]->clear();
  }

  mergePending.store(false);

  // Record the 9th loop into Slot 2
  setLoopLength(nextCaptureBars);
}

void FlowEngine::setSlotVolume(int slotIndex, float volume) {
  if (slotIndex >= 0 && slotIndex < (int)slots.size()) {
    slots[slotIndex]->setVolume(volume);
  }
}

void FlowEngine::setSlotMuted(int slotIndex, bool muted) {
  if (slotIndex >= 0 && slotIndex < (int)slots.size()) {
    slots[slotIndex]->setState(muted ? "MUTED" : "PLAYING");
  }
}

void FlowEngine::loadPreset(const juce::String &category,
                            const juce::String &presetName) {
  juce::Logger::writeToLog("Load Preset: " + category + " / " + presetName);

  sessionManager.updateState([&](AppState &s) {
    s.activeMode.category = category;
    s.activeMode.presetName = presetName;
    // Also update presetId to match name for now, or look it up
    s.activeMode.presetId = presetName.toLowerCase().replace(" ", "-");
  });

  // Apply preset to the appropriate engine
  if (category == "drums") {
    drumEngine.setKit(presetName);
  } else if (category == "notes" || category == "bass") {
    synthEngine.setPreset(category, presetName.toLowerCase().replace(" ", "-"));
  }
}

void FlowEngine::setActiveCategory(const juce::String &category) {
  juce::Logger::writeToLog("Set Active Category: " + category);

  // Apply default preset when switching categories and update state
  if (category == "drums") {
    drumEngine.setKit("synthetic");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.isFxMode = false;
      s.activeMode.presetId = "synthetic";
      s.activeMode.presetName = "Synthetic";
    });
  } else if (category == "notes") {
    synthEngine.setPreset("notes", "sine-bell");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.isFxMode = false;
      s.activeMode.presetId = "sine-bell";
      s.activeMode.presetName = "Sine Bell";
    });
  } else if (category == "bass") {
    synthEngine.setPreset("bass", "sub");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.isFxMode = false;
      s.activeMode.presetId = "sub";
      s.activeMode.presetName = "Sub";
    });
  } else {
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.isFxMode = (category == "fx" || category == "infinite_fx");
    });
  }
}

void FlowEngine::loadRiff(const juce::String &riffId) {
  juce::Logger::writeToLog("Load Riff: " + riffId);

  // TODO: Implement riff loading from history
  // For now, just log the request
  // In full implementation:
  // 1. Find riff in state.riffHistory by ID
  // 2. Load the riff's layer data into slots
  // 3. Update transport and playback state
}

void FlowEngine::triggerPad(int padIndex, float velocity) {
  juce::Logger::writeToLog("Pad Trigger: " + juce::String(padIndex) +
                           " vel=" + juce::String(velocity));

  // Pad index is actually the MIDI note number from the UI
  // For drums: 36-51 (GM drum mapping)
  // For notes/bass: 48-63 (C3-D#4)
  int midiNote = padIndex;

  // Add NOTE_ON to active MIDI buffer (will be processed in next processBlock)
  juce::MidiMessage noteOn =
      juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)(velocity * 127));
  activeMidi.addEvent(noteOn, 0);
}

void FlowEngine::releasePad(int padIndex) {
  juce::Logger::writeToLog("Pad Release: " + juce::String(padIndex));

  // Pad index is the MIDI note number
  int midiNote = padIndex;

  // Add NOTE_OFF to active MIDI buffer
  juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, midiNote);
  activeMidi.addEvent(noteOff, 0);
}

void FlowEngine::updateXY(float x, float y) {
  sessionManager.updateState([&](AppState &s) {
    s.activeFX.xyPosition.x = x;
    s.activeFX.xyPosition.y = y;
    s.activeFX.isActive = true;
  });
}

void FlowEngine::setInputGain(float gainDb) {
  micProcessor.setInputGain(gainDb);

  sessionManager.updateState([&](AppState &s) {
    // Store normalized value (0-1 range)
    s.mic.inputGain =
        (gainDb + 60.0f) / 100.0f; // Maps -60dB to +40dB → 0.0 to 1.0
  });
}

void FlowEngine::toggleMonitorInput() {
  auto state = sessionManager.getCurrentState();
  bool newValue = !state.mic.monitorInput;
  micProcessor.setMonitorEnabled(newValue);

  sessionManager.updateState(
      [&](AppState &s) { s.mic.monitorInput = newValue; });
}

void FlowEngine::toggleMonitorUntilLooped() {
  auto state = sessionManager.getCurrentState();
  bool newValue = !state.mic.monitorUntilLooped;
  micProcessor.setMonitorUntilLooped(newValue);

  sessionManager.updateState(
      [&](AppState &s) { s.mic.monitorUntilLooped = newValue; });
}

void FlowEngine::panic() {
  juce::Logger::writeToLog("PANIC - stopping all notes");

  // Stop drum engine
  drumEngine.reset();

  // Stop synth engine (all voices)
  synthEngine.reset();

  // Clear active MIDI buffer
  activeMidi.clear();
}

void FlowEngine::createNewJam() {
  juce::Logger::writeToLog("Create new jam");

  // Create a new session with a unique ID
  juce::String sessionId = juce::Uuid().toString();
  juce::String sessionName = "New Jam";

  sessionManager.updateState([&](AppState &s) {
    // Create new session entry
    AppState::Session newSession;
    newSession.id = sessionId;
    newSession.name = sessionName;
    newSession.emoji = "🎵";
    newSession.createdAt = juce::Time::currentTimeMillis();

    // If this is the very first session, ensure it's in the list
    bool exists = false;
    for (const auto &sess : s.sessions) {
      if (sess.id == sessionId) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      s.sessions.push_back(newSession);
    }

    // Set as current session
    s.session = newSession;

    // Reset slots
    for (auto &slot : s.slots) {
      slot.state = "EMPTY";
      slot.volume = 1.0f;
      slot.pluginChain.clear();
    }

    // Clear riff history for new session
    s.riffHistory.clear();
  });

  // Save state immediately (persistence fix for bd-p2l)
  auto appData =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone");
  sessionManager.saveSession(appData.getChildFile("last_session.flow"),
                             sessionManager.getCurrentState());
}

void FlowEngine::loadJam(const juce::String &sessionId) {
  juce::Logger::writeToLog("Load jam: " + sessionId);

  // TODO: Implement actual session loading from disk
  // For now, just update state to show we switched sessions
  sessionManager.updateState([&](AppState &s) {
    for (const auto &sess : s.sessions) {
      if (sess.id == sessionId) {
        s.session = sess;
        break;
      }
    }
  });
}

void FlowEngine::renameJam(const juce::String &sessionId,
                           const juce::String &name,
                           const juce::String &emoji) {
  juce::Logger::writeToLog("Rename jam: " + sessionId + " to " + name);

  sessionManager.updateState([&](AppState &s) {
    // Update in sessions list
    for (auto &sess : s.sessions) {
      if (sess.id == sessionId) {
        sess.name = name;
        if (emoji.isNotEmpty()) {
          sess.emoji = emoji;
        }

        // Also update current session if it matches
        if (s.session.id == sessionId) {
          s.session.name = name;
          if (emoji.isNotEmpty()) {
            s.session.emoji = emoji;
          }
        }
        break;
      }
    }
  });

  // Save state immediately (persistence fix for bd-p2l)
  auto appData =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone");
  sessionManager.saveSession(appData.getChildFile("last_session.flow"),
                             sessionManager.getCurrentState());
}

void FlowEngine::deleteJam(const juce::String &sessionId) {
  juce::Logger::writeToLog("Delete jam: " + sessionId);

  sessionManager.updateState([&](AppState &s) {
    // Remove from sessions list
    s.sessions.erase(std::remove_if(s.sessions.begin(), s.sessions.end(),
                                    [&](const AppState::Session &sess) {
                                      return sess.id == sessionId;
                                    }),
                     s.sessions.end());

    // If current session was deleted, create a new one
    if (s.session.id == sessionId) {
      juce::String newId = juce::Uuid().toString();
      s.session.id = newId;
      s.session.name = "New Jam";
      s.session.emoji = "🎵";
      s.session.createdAt = juce::Time::currentTimeMillis();
      s.sessions.push_back(s.session);
    }
  });
}

} // namespace flowzone
