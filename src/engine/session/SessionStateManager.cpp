#include "SessionStateManager.h"

namespace flowzone {

SessionStateManager::SessionStateManager() {}
SessionStateManager::~SessionStateManager() {}

juce::Result SessionStateManager::saveSession(const juce::File &file,
                                              const AppState &state) {
  auto jsonVar = state.toVar();
  auto jsonString = juce::JSON::toString(jsonVar);

  if (file.replaceWithText(jsonString)) {
    return juce::Result::ok();
  }

  return juce::Result::fail("Could not write validation file.");
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
  return juce::Result::ok();
}

juce::Result SessionStateManager::createNewSession(const juce::String &name,
                                                   const juce::File &parentDir,
                                                   AppState &outState) {
  // Basic init of state
  outState = AppState();
  outState.session.id = juce::Uuid().toString();
  outState.session.name = name;
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
  return saveSession(file, outState);
}

} // namespace flowzone
