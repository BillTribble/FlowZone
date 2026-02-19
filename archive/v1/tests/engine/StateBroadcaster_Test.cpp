#include "../../src/engine/state/AppState.h"
#include "../../src/engine/state/StateBroadcaster.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace flowzone;

TEST_CASE("StateBroadcaster Broadcasts", "[StateBroadcaster]") {
  StateBroadcaster broadcaster;

  // Capture dispatched message
  juce::String lastMessage;
  broadcaster.setMessageCallback(
      [&](const juce::String &msg) { lastMessage = msg; });

  SECTION("Broadcasts Full State") {
    AppState state;
    state.session.name = "Test Session";
    state.transport.bpm = 135.0;

    broadcaster.broadcastFullState(state);

    REQUIRE(lastMessage.isNotEmpty());

    // Parse back
    auto var = juce::JSON::parse(lastMessage);
    REQUIRE(var.isObject());

    std::string cmd = var["cmd"].toString().toStdString();
    REQUIRE(cmd == "STATE_FULL");

    REQUIRE(!var["revisionId"].isVoid());
    int revId = static_cast<int>(var["revisionId"]);
    REQUIRE(revId == 1);

    auto stateObj = var["state"];
    REQUIRE(stateObj.isObject());

    std::string sessionName =
        stateObj["session"]["name"].toString().toStdString();
    REQUIRE(sessionName == "Test Session");

    double bpm = static_cast<double>(stateObj["transport"]["bpm"]);
    REQUIRE(bpm == 135.0);
  }

  SECTION("Revision ID Increments") {
    AppState state;
    broadcaster.broadcastFullState(state);
    REQUIRE(broadcaster.getRevisionId() == 1);

    broadcaster.broadcastFullState(state);
    REQUIRE(broadcaster.getRevisionId() == 2);
  }
}
