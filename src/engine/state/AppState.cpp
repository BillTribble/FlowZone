#include "AppState.h"

namespace flowzone {

juce::var AppState::toVar() const {
  juce::DynamicObject *obj = new juce::DynamicObject();

  // Sessions array
  {
    juce::Array<juce::var> sessionsArr;
    for (const auto &sess : sessions) {
      juce::DynamicObject *sessObj = new juce::DynamicObject();
      sessObj->setProperty("id", sess.id);
      sessObj->setProperty("name", sess.name);
      sessObj->setProperty("emoji", sess.emoji);
      sessObj->setProperty("createdAt", sess.createdAt);
      sessionsArr.add(juce::var(sessObj));
    }
    obj->setProperty("sessions", sessionsArr);
  }

  // Session
  {
    juce::DynamicObject *sessionObj = new juce::DynamicObject();
    sessionObj->setProperty("id", session.id);
    sessionObj->setProperty("name", session.name);
    sessionObj->setProperty("emoji", session.emoji);
    sessionObj->setProperty("createdAt", session.createdAt);
    obj->setProperty("session", sessionObj);
  }

  // Transport
  {
    juce::DynamicObject *tObj = new juce::DynamicObject();
    tObj->setProperty("bpm", transport.bpm);
    tObj->setProperty("isPlaying", transport.isPlaying);
    tObj->setProperty("barPhase", transport.barPhase);
    tObj->setProperty("loopLengthBars", transport.loopLengthBars);
    tObj->setProperty("metronomeEnabled", transport.metronomeEnabled);
    tObj->setProperty("quantiseEnabled", transport.quantiseEnabled);
    tObj->setProperty("rootNote", transport.rootNote);
    tObj->setProperty("scale", transport.scale);
    obj->setProperty("transport", tObj);
  }

  // ActiveMode
  {
    juce::DynamicObject *amObj = new juce::DynamicObject();
    amObj->setProperty("category", activeMode.category);
    amObj->setProperty("presetId", activeMode.presetId);
    amObj->setProperty("presetName", activeMode.presetName);
    amObj->setProperty("isFxMode", activeMode.isFxMode);

    juce::Array<juce::var> slotsArr;
    for (int s : activeMode.selectedSourceSlots)
      slotsArr.add(s);
    amObj->setProperty("selectedSourceSlots", slotsArr);

    obj->setProperty("activeMode", amObj);
  }

  // ActiveFX
  {
    juce::DynamicObject *fxObj = new juce::DynamicObject();
    fxObj->setProperty("effectId", activeFX.effectId);
    fxObj->setProperty("effectName", activeFX.effectName);

    juce::DynamicObject *xyObj = new juce::DynamicObject();
    xyObj->setProperty("x", activeFX.xyPosition.x);
    xyObj->setProperty("y", activeFX.xyPosition.y);
    fxObj->setProperty("xyPosition", xyObj);

    fxObj->setProperty("isActive", activeFX.isActive);
    obj->setProperty("activeFX", fxObj);
  }

  // Mic
  {
    juce::DynamicObject *micObj = new juce::DynamicObject();
    micObj->setProperty("inputGain", mic.inputGain);
    micObj->setProperty("monitorInput", mic.monitorInput);
    micObj->setProperty("monitorUntilLooped", mic.monitorUntilLooped);
    obj->setProperty("mic", micObj);
  }

  // Slots
  {
    juce::Array<juce::var> slotsArr;
    for (const auto &slot : slots) {
      juce::DynamicObject *sObj = new juce::DynamicObject();
      sObj->setProperty("id", slot.id);
      sObj->setProperty("state", slot.state);
      sObj->setProperty("volume", slot.volume);
      sObj->setProperty("name", slot.name);
      sObj->setProperty("instrumentCategory", slot.instrumentCategory);
      sObj->setProperty("presetId", slot.presetId);
      sObj->setProperty("userId", slot.userId);
      sObj->setProperty("loopLengthBars", slot.loopLengthBars);
      sObj->setProperty("originalBpm", slot.originalBpm);
      if (slot.lastError != 0)
        sObj->setProperty("lastError", slot.lastError);

      juce::Array<juce::var> pluginsArr;
      for (const auto &p : slot.pluginChain) {
        juce::DynamicObject *pObj = new juce::DynamicObject();
        pObj->setProperty("id", p.id);
        pObj->setProperty("pluginId", p.pluginId);
        pObj->setProperty("name", p.name);
        pObj->setProperty("bypass", p.bypass);
        pluginsArr.add(pObj);
      }
      sObj->setProperty("pluginChain", pluginsArr);

      slotsArr.add(sObj);
    }
    obj->setProperty("slots", slotsArr);
  }

  // RiffHistory
  {
    juce::Array<juce::var> riffArr;
    for (const auto &r : riffHistory) {
      juce::DynamicObject *rObj = new juce::DynamicObject();
      rObj->setProperty("id", r.id);
      rObj->setProperty("timestamp", r.timestamp);
      rObj->setProperty("name", r.name);
      rObj->setProperty("layers", r.layers);
      rObj->setProperty("userId", r.userId);

      juce::Array<juce::var> colorsArr;
      for (const auto &c : r.colors)
        colorsArr.add(c);
      rObj->setProperty("colors", colorsArr);

      riffArr.add(rObj);
    }
    obj->setProperty("riffHistory", riffArr);
  }

  // Settings
  {
    juce::DynamicObject *setObj = new juce::DynamicObject();
    setObj->setProperty("riffSwapMode", settings.riffSwapMode);
    setObj->setProperty("bufferSize", settings.bufferSize);
    setObj->setProperty("sampleRate", settings.sampleRate);
    setObj->setProperty("storageLocation", settings.storageLocation);
    obj->setProperty("settings", setObj);
  }

  // System
  {
    juce::DynamicObject *sysObj = new juce::DynamicObject();
    sysObj->setProperty("cpuLoad", system.cpuLoad);
    sysObj->setProperty("diskBufferUsage", system.diskBufferUsage);
    sysObj->setProperty("memoryUsageMB", system.memoryUsageMB);
    sysObj->setProperty("activePluginHosts", system.activePluginHosts);
    obj->setProperty("system", sysObj);
  }

  // UI - Empty object as per Spec
  obj->setProperty("ui", new juce::DynamicObject());

  return juce::var(obj);
}

AppState AppState::fromVar(const juce::var &v) {
  AppState state;

  if (!v.isObject())
    return state;

  // Sessions array
  if (auto sessionsArr = v["sessions"]; sessionsArr.isArray()) {
    for (const auto &sVar : *sessionsArr.getArray()) {
      if (sVar.isObject()) {
        Session sess;
        sess.id = sVar["id"].toString();
        sess.name = sVar["name"].toString();
        sess.emoji = sVar["emoji"].toString();
        sess.createdAt = static_cast<int64_t>(sVar["createdAt"]);
        state.sessions.push_back(sess);
      }
    }
  }

  // Session
  if (auto sObj = v["session"]; sObj.isObject()) {
    state.session.id = sObj["id"].toString();
    state.session.name = sObj["name"].toString();
    state.session.emoji = sObj["emoji"].toString();
    state.session.createdAt = static_cast<int64_t>(sObj["createdAt"]);
  }

  // Transport
  if (auto tObj = v["transport"]; tObj.isObject()) {
    state.transport.bpm = static_cast<double>(tObj["bpm"]);
    state.transport.isPlaying = static_cast<bool>(tObj["isPlaying"]);
    state.transport.barPhase = static_cast<double>(tObj["barPhase"]);
    state.transport.loopLengthBars = static_cast<int>(tObj["loopLengthBars"]);
    state.transport.metronomeEnabled =
        static_cast<bool>(tObj["metronomeEnabled"]);
    state.transport.quantiseEnabled =
        static_cast<bool>(tObj["quantiseEnabled"]);
    state.transport.rootNote = static_cast<int>(tObj["rootNote"]);
    state.transport.scale = tObj["scale"].toString();
  }

  // ActiveMode
  if (auto amObj = v["activeMode"]; amObj.isObject()) {
    state.activeMode.category = amObj["category"].toString();
    state.activeMode.presetId = amObj["presetId"].toString();
    state.activeMode.presetName = amObj["presetName"].toString();
    state.activeMode.isFxMode = static_cast<bool>(amObj["isFxMode"]);

    if (auto slotsArr = amObj["selectedSourceSlots"]; slotsArr.isArray()) {
      for (auto &s : *slotsArr.getArray()) {
        state.activeMode.selectedSourceSlots.push_back(static_cast<int>(s));
      }
    }
  }

  // ActiveFX
  if (auto fxObj = v["activeFX"]; fxObj.isObject()) {
    state.activeFX.effectId = fxObj["effectId"].toString();
    state.activeFX.effectName = fxObj["effectName"].toString();
    state.activeFX.isActive = static_cast<bool>(fxObj["isActive"]);

    if (auto xyObj = fxObj["xyPosition"]; xyObj.isObject()) {
      state.activeFX.xyPosition.x = static_cast<float>(xyObj["x"]);
      state.activeFX.xyPosition.y = static_cast<float>(xyObj["y"]);
    }
  }

  // Mic
  if (auto micObj = v["mic"]; micObj.isObject()) {
    state.mic.inputGain = static_cast<float>(micObj["inputGain"]);
    state.mic.monitorInput = static_cast<bool>(micObj["monitorInput"]);
    state.mic.monitorUntilLooped =
        static_cast<bool>(micObj["monitorUntilLooped"]);
  }

  // Slots
  if (auto slotsArr = v["slots"]; slotsArr.isArray()) {
    for (auto &sVal : *slotsArr.getArray()) {
      SlotState slot;
      slot.id = sVal["id"].toString();
      slot.state = sVal["state"].toString();
      slot.volume = static_cast<float>(sVal["volume"]);
      slot.name = sVal["name"].toString();
      slot.instrumentCategory = sVal["instrumentCategory"].toString();
      slot.presetId = sVal["presetId"].toString();
      slot.userId = sVal["userId"].toString();
      slot.loopLengthBars = static_cast<int>(sVal["loopLengthBars"]);
      slot.originalBpm = static_cast<double>(sVal["originalBpm"]);
      if (sVal.hasProperty("lastError"))
        slot.lastError = static_cast<int>(sVal["lastError"]);

      if (auto pArr = sVal["pluginChain"]; pArr.isArray()) {
        for (auto &pVal : *pArr.getArray()) {
          PluginInstance p;
          p.id = pVal["id"].toString();
          p.pluginId = pVal["pluginId"].toString();
          p.name = pVal["name"].toString();
          p.bypass = static_cast<bool>(pVal["bypass"]);
          slot.pluginChain.push_back(p);
        }
      }
      state.slots.push_back(slot);
    }
  }

  // RiffHistory
  if (auto riffArr = v["riffHistory"]; riffArr.isArray()) {
    for (auto &rVal : *riffArr.getArray()) {
      RiffHistoryEntry r;
      r.id = rVal["id"].toString();
      r.timestamp = static_cast<int64_t>(rVal["timestamp"]);
      r.name = rVal["name"].toString();
      r.layers = static_cast<int>(rVal["layers"]);
      r.userId = rVal["userId"].toString();

      if (auto cArr = rVal["colors"]; cArr.isArray()) {
        for (auto &c : *cArr.getArray()) {
          r.colors.push_back(c.toString());
        }
      }
      state.riffHistory.push_back(r);
    }
  }

  // Settings
  if (auto setObj = v["settings"]; setObj.isObject()) {
    state.settings.riffSwapMode = setObj["riffSwapMode"].toString();
    state.settings.bufferSize = static_cast<int>(setObj["bufferSize"]);
    state.settings.sampleRate = static_cast<double>(setObj["sampleRate"]);
    state.settings.storageLocation = setObj["storageLocation"].toString();
  }

  // System
  if (auto sysObj = v["system"]; sysObj.isObject()) {
    state.system.cpuLoad = static_cast<float>(sysObj["cpuLoad"]);
    state.system.diskBufferUsage =
        static_cast<float>(sysObj["diskBufferUsage"]);
    state.system.memoryUsageMB = static_cast<float>(sysObj["memoryUsageMB"]);
    state.system.activePluginHosts =
        static_cast<int>(sysObj["activePluginHosts"]);
  }

  return state;
}

} // namespace flowzone
