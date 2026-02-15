#include "CommandDispatcher.h"
#include "FlowEngine.h"

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
  } else if (cmdType == "NOTE_ON") {
    handleNoteOn(engine, (int)jsonVar["pad"], (float)jsonVar["val"]);
  } else if (cmdType == "XY_CHANGE") {
    handleXYChange(engine, (float)jsonVar["x"], (float)jsonVar["y"]);
  }
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

void CommandDispatcher::handleNoteOn(FlowEngine &engine, int pad,
                                     float velocity) {
  engine.triggerPad(pad, velocity);
}

void CommandDispatcher::handleXYChange(FlowEngine &engine, float x, float y) {
  engine.updateXY(x, y);
}

} // namespace flowzone
