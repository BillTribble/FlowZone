#include "CommandDispatcher.h"
#include "FlowEngine.h"
#include <JuceHeader.h>
#include <string>

namespace flowzone {

CommandDispatcher::CommandDispatcher() {}
CommandDispatcher::~CommandDispatcher() {}

void CommandDispatcher::dispatch(const juce::String &jsonCommand,
                                 FlowEngine &engine) {
  auto jsonVar = juce::JSON::parse(jsonCommand);
  if (!jsonVar.isObject())
    return;

  auto cmdType = jsonVar["cmd"].toString();

  if (cmdType == "PLAY") {
    handlePlay(engine);
  } else if (cmdType == "PAUSE") {
    handlePause(engine);
  } else if (cmdType == "TOGGLE_PLAY") {
    handleTogglePlay(engine);
  } else if (cmdType == "TOGGLE_METRONOME") {
    handleToggleMetronome(engine);
  } else if (cmdType == "SET_TEMPO") {
    // Check if bpm exists
    if (jsonVar.hasProperty("bpm")) {
      handleSetBpm(engine, (double)jsonVar["bpm"]);
    }
  } else if (cmdType == "SET_PRESET") {
    handleSetPreset(engine, jsonVar["category"].toString(),
                    jsonVar["preset"].toString());
  } else if (cmdType == "SET_MODE") {
    handleSetMode(engine, jsonVar["category"].toString());
  } else if (cmdType == "LOAD_RIFF") {
    handleLoadRiff(engine, jsonVar["riffId"].toString());
  } else if (cmdType == "NOTE_ON") {
    handleNoteOn(engine, (int)jsonVar["pad"], (float)jsonVar["val"]);
  } else if (cmdType == "NOTE_OFF") {
    handleNoteOff(engine, (int)jsonVar["pad"]);
  } else if (cmdType == "XY_CHANGE") {
    handleXYChange(engine, (float)jsonVar["x"], (float)jsonVar["y"]);
  } else if (cmdType == "SET_LOOP_LENGTH") {
    handleSetLoopLength(engine, (int)jsonVar["bars"]);
  } else if (cmdType == "MUTE_SLOT") {
    handleSetSlotMuted(engine, (int)jsonVar["index"], true);
  } else if (cmdType == "UNMUTE_SLOT") {
    handleSetSlotMuted(engine, (int)jsonVar["index"], false);
  } else if (cmdType == "SET_VOL") {
    handleSetSlotVolume(engine, (int)jsonVar["index"], (float)jsonVar["val"]);
  } else if (cmdType == "SET_INPUT_GAIN") {
    handleSetInputGain(engine, (float)jsonVar["val"]);
  } else if (cmdType == "TOGGLE_MONITOR_INPUT") {
    handleToggleMonitorInput(engine);
  } else if (cmdType == "TOGGLE_MONITOR_UNTIL_LOOPED") {
    handleToggleMonitorUntilLooped(engine);
  } else if (cmdType == "PANIC") {
    handlePanic(engine);
  } else if (cmdType == "NEW_JAM") {
    handleNewJam(engine);
  } else if (cmdType == "LOAD_JAM") {
    handleLoadJam(engine, jsonVar["sessionId"].toString());
  } else if (cmdType == "RENAME_JAM") {
    handleRenameJam(
        engine, jsonVar["sessionId"].toString(), jsonVar["name"].toString(),
        jsonVar.hasProperty("emoji") ? jsonVar["emoji"].toString() : "");
  } else if (cmdType == "DELETE_JAM") {
    handleDeleteJam(engine, jsonVar["sessionId"].toString());
  } else if (cmdType == "COMMIT") {
    handleCommit(engine);
  }
}

void CommandDispatcher::handleCommit(FlowEngine &engine) {
  DBG("[CommandDispatcher] COMMIT retrospective audio to slot");
  engine.commitLooper();
}

void CommandDispatcher::handlePlay(FlowEngine &engine) {
  engine.getTransport().play();
}

void CommandDispatcher::handlePause(FlowEngine &engine) {
  engine.getTransport().pause();
}

void CommandDispatcher::handleTogglePlay(FlowEngine &engine) {
  engine.getTransport().togglePlay();
}

void CommandDispatcher::handleToggleMetronome(FlowEngine &engine) {
  bool current = engine.getTransport().isMetronomeEnabled();
  engine.getTransport().setMetronomeEnabled(!current);
}

void CommandDispatcher::handleSetBpm(FlowEngine &engine, double bpm) {
  engine.getTransport().setBpm(bpm);
}

void CommandDispatcher::handleSetPreset(FlowEngine &engine,
                                        const juce::String &category,
                                        const juce::String &preset) {
  // Forward to Engine to handle preset loading logic
  engine.loadPreset(category, preset);
}

void CommandDispatcher::handleSetMode(FlowEngine &engine,
                                      const juce::String &category) {
  // Set the active mode category (drums, notes, bass, fx, etc.)
  DBG("[CommandDispatcher] Set mode to category: " + category);
  engine.setActiveCategory(category);
}

void CommandDispatcher::handleLoadRiff(FlowEngine &engine,
                                       const juce::String &riffId) {
  // Load a riff from history
  DBG("[CommandDispatcher] Load riff: " + riffId);
  engine.loadRiff(riffId);
}

void CommandDispatcher::handleNoteOn(FlowEngine &engine, int pad,
                                     float velocity) {
  engine.triggerPad(pad, velocity);
}

void CommandDispatcher::handleNoteOff(FlowEngine &engine, int pad) {
  engine.releasePad(pad);
}

void CommandDispatcher::handleXYChange(FlowEngine &engine, float x, float y) {
  engine.updateXY(x, y);
}

void CommandDispatcher::handleSetLoopLength(FlowEngine &engine, int bars) {
  engine.setLoopLength(bars);
}

void CommandDispatcher::handleSetSlotMuted(FlowEngine &engine, int index,
                                           bool muted) {
  engine.setSlotMuted(index, muted);
}

void CommandDispatcher::handleSetSlotVolume(FlowEngine &engine, int index,
                                            float volume) {
  engine.setSlotVolume(index, volume);
}

void CommandDispatcher::handleSetInputGain(FlowEngine &engine, float gainDb) {
  engine.setInputGain(gainDb);
}

void CommandDispatcher::handleToggleMonitorInput(FlowEngine &engine) {
  engine.toggleMonitorInput();
}

void CommandDispatcher::handleToggleMonitorUntilLooped(FlowEngine &engine) {
  engine.toggleMonitorUntilLooped();
}

void CommandDispatcher::handlePanic(FlowEngine &engine) {
  DBG("[CommandDispatcher] PANIC - stopping all notes");
  engine.panic();
}

void CommandDispatcher::handleNewJam(FlowEngine &engine) {
  DBG("[CommandDispatcher] Create new jam");
  engine.createNewJam();
}

void CommandDispatcher::handleLoadJam(FlowEngine &engine,
                                      const juce::String &sessionId) {
  DBG("[CommandDispatcher] Load jam: " + sessionId);
  engine.loadJam(sessionId);
}

void CommandDispatcher::handleRenameJam(FlowEngine &engine,
                                        const juce::String &sessionId,
                                        const juce::String &name,
                                        const juce::String &emoji) {
  DBG("[CommandDispatcher] Rename jam: " + sessionId + " to " + name);
  engine.renameJam(sessionId, name, emoji);
}

void CommandDispatcher::handleDeleteJam(FlowEngine &engine,
                                        const juce::String &sessionId) {
  DBG("[CommandDispatcher] Delete jam: " + sessionId);
  engine.deleteJam(sessionId);
}

} // namespace flowzone
