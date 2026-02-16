#include "SessionStateManager.h"

namespace flowzone {

SessionStateManager::SessionStateManager() {}

SessionStateManager::~SessionStateManager() {
  stopAutosave();
}

juce::Result SessionStateManager::saveSession(const juce::File &file,
                                              const AppState &state) {
  auto jsonVar = state.toVar();
  auto jsonString = juce::JSON::toString(jsonVar);

  if (file.replaceWithText(jsonString)) {
    logSessionEvent("SAVE_SESSION", file.getFullPathName());
    return juce::Result::ok();
  }

  return juce::Result::fail("Could not write session file.");
}

juce::Result SessionStateManager::loadSession(const juce::File &file,
                                              AppState &outState) {
  if (!file.existsAsFile()) {
    return juce::Result::fail("File does not exist: " + file.getFullPathName());
  }

  auto jsonString = file.loadFileAsString();
  auto var = juce::JSON::parse(jsonString);

  if (var.isVoid()) {
    return juce::Result::fail("Failed to parse JSON.");
  }

  outState = AppState::fromVar(var);
  logSessionEvent("LOAD_SESSION", file.getFullPathName());
  return juce::Result::ok();
}

juce::Result SessionStateManager::createNewSession(const juce::String &name,
                                                   const juce::File &parentDir,
                                                   AppState &outState) {
  // Basic init of state
  outState = AppState();
  outState.session.id = juce::Uuid().toString();
  outState.session.name = name;
  outState.session.emoji = "🎵";
  outState.session.createdAt = juce::Time::currentTimeMillis();

  // Create directory
  auto sessionDir = parentDir.getChildFile(name);
  if (!sessionDir.exists()) {
    if (!sessionDir.createDirectory()) {
      return juce::Result::fail("Could not create session directory.");
    }
  }

  // Save initial state
  auto file = sessionDir.getChildFile("session.flow");
  auto result = saveSession(file, outState);
  
  if (result.wasOk()) {
    logSessionEvent("CREATE_SESSION", "id=" + outState.session.id + ", name=" + name);
  }
  
  return result;
}

void SessionStateManager::startAutosave(const juce::File &sessionDir) {
  juce::ScopedLock lock(stateLock);
  autosaveDir = sessionDir;
  autosaveEnabled = true;
  
  // Create autosave directory if needed
  auto autosaveFolder = autosaveDir.getChildFile("autosave");
  if (!autosaveFolder.exists()) {
    autosaveFolder.createDirectory();
  }
  
  // Start timer: 30 seconds interval
  startTimer(30000);
  
  logSessionEvent("AUTOSAVE_STARTED", sessionDir.getFullPathName());
}

void SessionStateManager::stopAutosave() {
  stopTimer();
  autosaveEnabled = false;
  logSessionEvent("AUTOSAVE_STOPPED");
}

juce::Result SessionStateManager::performAutosave() {
  if (!autosaveEnabled) {
    return juce::Result::fail("Autosave not enabled");
  }
  
  juce::ScopedLock lock(stateLock);
  
  auto autosaveFolder = autosaveDir.getChildFile("autosave");
  auto timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
  timestamp = timestamp.replaceCharacter(':', '-').replaceCharacter(' ', '_');
  
  auto autosaveFile = autosaveFolder.getChildFile("autosave_" + timestamp + ".flow");
  
  auto result = saveSession(autosaveFile, currentState);
  
  if (result.wasOk()) {
    // Keep only last 5 autosaves
    auto autosaves = autosaveFolder.findChildFiles(juce::File::findFiles, false, "autosave_*.flow");
    autosaves.sort();
    
    while (autosaves.size() > 5) {
      autosaves[0].deleteFile();
      autosaves.remove(0);
    }
    
    logSessionEvent("AUTOSAVE", autosaveFile.getFileName());
  }
  
  return result;
}

void SessionStateManager::timerCallback() {
  performAutosave();
}

juce::Result SessionStateManager::loadLastAutosave(const juce::File &sessionDir,
                                                   AppState &outState) {
  auto autosaveFolder = sessionDir.getChildFile("autosave");
  
  if (!autosaveFolder.exists()) {
    return juce::Result::fail("No autosave directory found");
  }
  
  auto autosaves = autosaveFolder.findChildFiles(juce::File::findFiles, false, "autosave_*.flow");
  
  if (autosaves.isEmpty()) {
    return juce::Result::fail("No autosave files found");
  }
  
  // Sort by timestamp (newest last)
  autosaves.sort();
  auto latest = autosaves.getLast();
  
  auto result = loadSession(latest, outState);
  
  if (result.wasOk()) {
    logSessionEvent("CRASH_RECOVERY", latest.getFileName());
  }
  
  return result;
}

bool SessionStateManager::hasAutosave(const juce::File &sessionDir) const {
  auto autosaveFolder = sessionDir.getChildFile("autosave");
  
  if (!autosaveFolder.exists()) {
    return false;
  }
  
  auto autosaves = autosaveFolder.findChildFiles(juce::File::findFiles, false, "autosave_*.flow");
  return !autosaves.isEmpty();
}

juce::Result SessionStateManager::commitRiff(const juce::String &name,
                                            int layers,
                                            const std::vector<juce::String> &colors,
                                            const juce::String &userId) {
  juce::ScopedLock lock(stateLock);
  
  RiffHistoryEntry entry;
  entry.id = juce::Uuid().toString();
  entry.timestamp = juce::Time::currentTimeMillis();
  entry.name = name;
  entry.layers = layers;
  entry.colors = colors;
  entry.userId = userId;
  
  currentState.riffHistory.push_back(entry);
  
  logSessionEvent("COMMIT_RIFF",
                  "id=" + entry.id + ", layers=" + juce::String(layers) +
                  ", colors=" + juce::String(colors.size()) + ", user=" + userId);
  
  return juce::Result::ok();
}

juce::Result SessionStateManager::loadRiff(const juce::String &riffId) {
  juce::ScopedLock lock(stateLock);
  
  // Find riff in history
  for (const auto &riff : currentState.riffHistory) {
    if (riff.id == riffId) {
      logSessionEvent("LOAD_RIFF", "id=" + riffId + ", name=" + riff.name);
      
      // In full implementation, would restore slot states from riff data
      // For now, just log the event
      return juce::Result::ok();
    }
  }
  
  return juce::Result::fail("Riff not found: " + riffId);
}

juce::Result SessionStateManager::deleteJam(const juce::String &sessionId) {
  juce::ScopedLock lock(stateLock);
  
  if (autosaveDir.exists()) {
    // Delete session directory and all contents
    if (autosaveDir.deleteRecursively()) {
      logSessionEvent("DELETE_JAM", "sessionId=" + sessionId);
      
      // Clear current state
      currentState = AppState();
      
      return juce::Result::ok();
    }
    
    return juce::Result::fail("Failed to delete session directory");
  }
  
  return juce::Result::fail("Session directory not found");
}

void SessionStateManager::updateState(std::function<void(AppState &)> modifier) {
  juce::ScopedLock lock(stateLock);
  modifier(currentState);
}

void SessionStateManager::logSessionEvent(const juce::String &event,
                                         const juce::String &details) {
  auto timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
  juce::String logEntry = timestamp + " - SessionManager: " + event;
  
  if (details.isNotEmpty()) {
    logEntry += " - " + details;
  }
  
  DBG(logEntry);
  
  // In production, write to log file
  // ~/Library/Logs/FlowZone/engine.jsonl
}

} // namespace flowzone
