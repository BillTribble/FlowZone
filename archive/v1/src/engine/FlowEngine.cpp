#include "FlowEngine.h"
#include "FileLogger.h"
#include <cmath>

namespace flowzone {

FlowEngine::FlowEngine()
    : juce::Thread("AutoMergeThread"), transport(), dispatcher(), broadcaster(),
      sessionManager(), retroBuffer(), featureExtractor(), commandQueue() {

  FileLogger::instance().log(FileLogger::Category::Startup,
                             "FlowEngine constructor START");

  // Create 12 empty slots
  for (int i = 0; i < 12; ++i) {
    auto slot = std::make_unique<Slot>(i);
    slots.push_back(std::move(slot));
  }

  createNewJam();
  transport.play(); // Auto-play when opening a jam
  startTimerHz(60); // Broadcast state at 60Hz

  FileLogger::instance().log(FileLogger::Category::Startup,
                             "FlowEngine constructor DONE, transport playing");
}

FlowEngine::~FlowEngine() {
  FileLogger::instance().log(FileLogger::Category::Startup,
                             "FlowEngine SHUTDOWN");
  stopThread(2000);
}

void FlowEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;

  FileLogger::instance().log(FileLogger::Category::Startup,
                             "prepareToPlay sr=" + std::to_string(sampleRate) +
                                 " block=" + std::to_string(samplesPerBlock));

  transport.prepareToPlay(sampleRate, samplesPerBlock);
  drumEngine.prepare(sampleRate, samplesPerBlock);
  synthEngine.prepare(sampleRate, samplesPerBlock);
  micProcessor.prepare(sampleRate, samplesPerBlock);

  // Set default mic gain to match AppState default of 0.67 (~7 dB)
  float defaultGainDb = (0.67f * 100.0f) - 60.0f; // ~7 dB
  micProcessor.setInputGain(defaultGainDb);

  retroBuffer.prepare(sampleRate, 60); // 60 seconds retrospective buffer
  featureExtractor.prepare(sampleRate, samplesPerBlock);

  engineBuffer.setSize(2, samplesPerBlock);
  retroCaptureBuffer.setSize(2, samplesPerBlock);

  for (auto &slot : slots) {
    slot->prepareToPlay(sampleRate, samplesPerBlock);
  }
}

void FlowEngine::processBlock(juce::AudioBuffer<float> &buffer,
                              juce::MidiBuffer &midiMessages) {
  processCommands();

  int numSamples = buffer.getNumSamples();
  engineBuffer.clear();
  retroCaptureBuffer.clear();

  // 1. Process Active Mode (Input/Synth)
  auto state = sessionManager.getCurrentState();

  // Combine UI MIDI with any incoming MIDI
  juce::MidiBuffer combinedMidi;
  combinedMidi.addEvents(midiMessages, 0, numSamples, 0);
  combinedMidi.addEvents(activeMidi, 0, numSamples, 0);
  activeMidi.clear();

  static int flowLogCounter = 0;
  bool shouldLog = (++flowLogCounter >= 86); // ~1/sec
  if (shouldLog)
    flowLogCounter = 0;

  if (state.activeMode.category == "mic") {
    micProcessor.process(buffer, engineBuffer);

    // After processing mic, clear the main buffer to remove raw input/noise
    buffer.clear();

    for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                     ch < engineBuffer.getNumChannels();
         ++ch) {
      retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0, numSamples);
    }

    if (state.mic.monitorInput || state.mic.monitorUntilLooped) {
      for (int ch = 0;
           ch < buffer.getNumChannels() && ch < engineBuffer.getNumChannels();
           ++ch) {
        buffer.copyFrom(ch, 0, engineBuffer, ch, 0, numSamples);
      }
    }
  } else {
    // If not in mic mode, still clear the main buffer to remove any raw mic
    // bleed
    buffer.clear();

    if (state.activeMode.category == "drums") {
      drumEngine.process(engineBuffer, combinedMidi);
      for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                       ch < engineBuffer.getNumChannels();
           ++ch) {
        retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0, numSamples);
      }
    } else if (state.activeMode.category == "notes" ||
               state.activeMode.category == "bass") {
      synthEngine.process(engineBuffer, combinedMidi);
      for (int ch = 0; ch < retroCaptureBuffer.getNumChannels() &&
                       ch < engineBuffer.getNumChannels();
           ++ch) {
        retroCaptureBuffer.copyFrom(ch, 0, engineBuffer, ch, 0, numSamples);
      }
    }

    // Add synth/drums to the now-clear buffer
    for (int ch = 0;
         ch < buffer.getNumChannels() && ch < engineBuffer.getNumChannels();
         ++ch) {
      buffer.copyFrom(ch, 0, engineBuffer, ch, 0, numSamples);
    }
  }

  // 2. Process Slots (Looped Riffs)
  for (auto &slot : slots) {
    slot->processBlock(buffer, numSamples);
  }

  // 3. Update Visuals & Retrospective
  updatePeakLevel(retroCaptureBuffer);
  retroBuffer.pushBlock(retroCaptureBuffer);
  featureExtractor.pushAudioBlock(retroCaptureBuffer);

  if (shouldLog) {
    float enginePeak = engineBuffer.getMagnitude(0, 0, numSamples);
    float retroPeak = retroBufferPeakLevel.load();
    FileLogger::instance().log(FileLogger::Category::AudioFlow,
                               "ENGINE peak=" + std::to_string(enginePeak) +
                                   " RETRO peak=" + std::to_string(retroPeak));
  }
}

void FlowEngine::loadPreset(const juce::String &category,
                            const juce::String &presetName) {
  sessionManager.updateState([&](AppState &s) {
    s.activeMode.category = category;
    s.activeMode.presetName = presetName;
    s.activeMode.presetId = presetName.toLowerCase().replace(" ", "-");
  });

  if (category == "drums") {
    drumEngine.setKit(presetName);
  } else if (category == "notes" || category == "bass") {
    synthEngine.setPreset(category, presetName.toLowerCase().replace(" ", "-"));
  }
}

void FlowEngine::setActiveCategory(const juce::String &category) {
  if (category == "drums") {
    drumEngine.setKit("synthetic");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.presetId = "synthetic";
      s.activeMode.presetName = "Synthetic";
    });
  } else if (category == "notes") {
    synthEngine.setPreset("notes", "sine-bell");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
      s.activeMode.presetId = "sine-bell";
      s.activeMode.presetName = "Sine Bell";
    });
  } else if (category == "bass") {
    synthEngine.setPreset("bass", "sub");
    sessionManager.updateState([&](AppState &s) {
      s.activeMode.category = category;
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
  // TODO
}

void FlowEngine::triggerPad(int padIndex, float velocity) {
  juce::MidiMessage noteOn =
      juce::MidiMessage::noteOn(1, padIndex, (juce::uint8)(velocity * 127));
  activeMidi.addEvent(noteOn, 0);
}

void FlowEngine::releasePad(int padIndex) {
  juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, padIndex);
  activeMidi.addEvent(noteOff, 0);
}

void FlowEngine::updateXY(float x, float y) {
  sessionManager.updateState([&](AppState &s) {
    s.activeFX.xyPosition.x = x;
    s.activeFX.xyPosition.y = y;
  });
}

void FlowEngine::setLoopLength(int bars) {
  transport.setLoopLengthBars(bars);
  sessionManager.updateState(
      [&](AppState &s) { s.transport.loopLengthBars = bars; });
}

void FlowEngine::setSlotVolume(int slotIndex, float volume) {
  if (slotIndex >= 0 && slotIndex < slots.size()) {
    slots[slotIndex]->setVolume(volume);
    sessionManager.updateState([&](AppState &s) {
      if (slotIndex < s.slots.size())
        s.slots[slotIndex].volume = volume;
    });
  }
}

void FlowEngine::setSlotMuted(int slotIndex, bool muted) {
  if (slotIndex >= 0 && slotIndex < slots.size()) {
    slots[slotIndex]->setMuted(muted);
    sessionManager.updateState([&](AppState &s) {
      if (slotIndex < s.slots.size())
        s.slots[slotIndex].muted = muted;
    });
  }
}

void FlowEngine::setInputGain(float gainDb) {
  micProcessor.setInputGain(gainDb);
  sessionManager.updateState(
      [&](AppState &s) { s.mic.inputGain = (gainDb + 60.0f) / 100.0f; });
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

void FlowEngine::panic() { activeMidi.clear(); }

void FlowEngine::commitLooper() {
  int bars = transport.getLoopLengthBars();
  if (bars <= 0)
    bars = 4;
  double samplesPerQuarter = 60.0 / transport.getBpm() * currentSampleRate;
  int totalSamples = (int)(bars * 4 * samplesPerQuarter);

  juce::AudioBuffer<float> tempBuffer;
  retroBuffer.getPastAudio(0, totalSamples, tempBuffer);

  auto state = sessionManager.getCurrentState();
  int targetSlot = -1;
  for (int i = 0; i < state.slots.size(); ++i) {
    if (state.slots[i].riffId.isEmpty()) {
      targetSlot = i;
      break;
    }
  }
  if (targetSlot == -1)
    targetSlot = 0;

  juce::String riffId = "commit_" + juce::Uuid().toString();
  state.slots[targetSlot].riffId = riffId;
  state.slots[targetSlot].volume = 1.0f;
  state.slots[targetSlot].muted = false;

  // Actually put the audio in the slot
  slots[targetSlot]->setAudioData(tempBuffer);

  sessionManager.setState(state);
}

void FlowEngine::createNewJam() {
  // Preserve existing sessions list
  auto existingState = sessionManager.getCurrentState();

  AppState newState;
  newState.session.id = juce::Uuid().toString();
  newState.session.name = "New Jam";
  newState.session.createdAt = juce::Time::currentTimeMillis();
  newState.activeMode.category = "drums";
  for (int i = 0; i < 12; ++i)
    newState.slots.push_back({});

  // Copy existing sessions and add the new one
  newState.sessions = existingState.sessions;
  AppState::Session sessionEntry;
  sessionEntry.id = newState.session.id;
  sessionEntry.name = newState.session.name;
  sessionEntry.createdAt = newState.session.createdAt;
  newState.sessions.push_back(sessionEntry);

  sessionManager.setState(newState);
  transport.play(); // Auto-play new jams
}

void FlowEngine::loadJam(const juce::String &sessionId) {}
void FlowEngine::renameJam(const juce::String &sessionId,
                           const juce::String &name,
                           const juce::String &emoji) {}
void FlowEngine::deleteJam(const juce::String &sessionId) {}
void FlowEngine::run() {}

void FlowEngine::broadcastState() {
  auto state = sessionManager.getCurrentState();
  state.mic.inputLevel = micProcessor.getPeakLevel();
  state.looper.inputLevel = retroBufferPeakLevel.load();
  state.looper.waveformData = retroBuffer.getWaveformData(256);
  state.transport.isPlaying = transport.isPlaying();
  state.transport.bpm = transport.getBpm();
  state.transport.barPhase = transport.getBarPhase();
  state.transport.metronomeEnabled = transport.isMetronomeEnabled();
  state.transport.loopLengthBars = transport.getLoopLengthBars();
  broadcaster.broadcastStateUpdate(state);

  // Sampled logging for state broadcast debugging (~1/sec at 60Hz)
  static int broadcastLogCounter = 0;
  if (++broadcastLogCounter >= 60) {
    broadcastLogCounter = 0;
    float retroPeak = retroBufferPeakLevel.load();
    float micPeak = micProcessor.getPeakLevel();
    bool hasWaveform = false;
    for (float v : state.looper.waveformData) {
      if (v > 0.001f) {
        hasWaveform = true;
        break;
      }
    }
    FileLogger::instance().log(
        FileLogger::Category::StateBroadcast,
        "BROADCAST retroPeak=" + std::to_string(retroPeak) +
            " micPeak=" + std::to_string(micPeak) +
            " playing=" + std::string(state.transport.isPlaying ? "Y" : "N") +
            " mode=" + state.activeMode.category.toStdString() +
            " waveform=" + std::string(hasWaveform ? "HAS_DATA" : "EMPTY") +
            " sessions=" + std::to_string(state.sessions.size()));
  }
}

void FlowEngine::updatePeakLevel(const juce::AudioBuffer<float> &buffer) {
  float peak = 0.0f;
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
  }
  float cur = retroBufferPeakLevel.load();
  if (peak > cur)
    retroBufferPeakLevel.store(peak);
  else
    retroBufferPeakLevel.store(cur * 0.95f);
}

void FlowEngine::processCommands() {
  juce::String cmd;
  while (commandQueue.pop(cmd)) {
    dispatcher.dispatch(cmd, *this);
  }
}

} // namespace flowzone
