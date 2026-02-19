#include "../../src/engine/session/SessionStateManager.h"
#include "../../src/engine/state/AppState.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone;

TEST_CASE("SessionStateManager Load/Save", "[SessionStateManager]") {
  SessionStateManager manager;

  // Create temp file
  juce::File tempFile =
      juce::File::getSpecialLocation(juce::File::tempDirectory)
          .getChildFile("flowzone_test_session.json");

  if (tempFile.exists())
    tempFile.deleteFile();

  SECTION("Save and Load") {
    AppState originalState;
    originalState.session.name = "Persistent Session";
    originalState.transport.bpm = 142.0;
    originalState.slots.push_back({});
    originalState.slots[0].name = "Test Slot";

    auto result = manager.saveSession(tempFile, originalState);
    REQUIRE(result.wasOk());
    REQUIRE(tempFile.existsAsFile());

    AppState loadedState;
    result = manager.loadSession(tempFile, loadedState);
    REQUIRE(result.wasOk());

    REQUIRE(loadedState.session.name.toStdString() == "Persistent Session");
    REQUIRE(loadedState.transport.bpm == 142.0);
    REQUIRE(loadedState.slots.size() == 1);
    REQUIRE(loadedState.slots[0].name.toStdString() == "Test Slot");
  }

  // Cleanup
  if (tempFile.exists())
    tempFile.deleteFile();
}
