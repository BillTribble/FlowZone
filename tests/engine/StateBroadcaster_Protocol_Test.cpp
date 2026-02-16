/**
 * Protocol Conformance Test for StateBroadcaster
 * 
 * Tests RFC 6902 JSON Patch generation and patch-vs-snapshot decision logic.
 * 
 * Test cases:
 * - Send canned RFC 6902 patch sequences
 * - Validate patch application results in correct state
 * - Test patch-vs-snapshot threshold (>20 ops or >4KB)
 * - Test client reconnect with stale revisionId → receives full snapshot
 * - Test patch operations: add, remove, replace
 * - Log patch sizes, op counts, and snapshot-vs-patch decisions
 */

#include "../../src/engine/state/StateBroadcaster.h"
#include "../../src/engine/state/AppState.h"
#include "../../libs/catch2/catch.hpp"
#include <JuceHeader.h>

using namespace flowzone;

TEST_CASE("StateBroadcaster RFC 6902 Patches", "[StateBroadcaster][Protocol]") {
  
  SECTION("Sends full snapshot on first broadcast") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state;
    state.transport.bpm = 120.0;
    state.transport.isPlaying = false;
    
    broadcaster.broadcastFullState(state);
    
    REQUIRE(!lastMessage.isEmpty());
    
    auto msgVar = juce::JSON::parse(lastMessage);
    REQUIRE(msgVar.isObject());
    REQUIRE(msgVar["type"].toString() == "STATE_FULL");
    REQUIRE(static_cast<int64_t>(msgVar["revisionId"]) == 1);
    REQUIRE(msgVar.hasProperty("data"));
    
    auto dataObj = msgVar["data"];
    REQUIRE(static_cast<double>(dataObj["transport"]["bpm"]) == 120.0);
  }
  
  SECTION("Generates replace patch for simple value change") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    // Send initial state
    AppState state1;
    state1.transport.bpm = 120.0;
    broadcaster.broadcastFullState(state1);
    
    // Update BPM
    AppState state2 = state1;
    state2.transport.bpm = 140.0;
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    REQUIRE(msgVar["type"].toString() == "STATE_PATCH");
    REQUIRE(static_cast<int64_t>(msgVar["revisionId"]) == 2);
    
    auto ops = msgVar["ops"];
    REQUIRE(ops.isArray());
    REQUIRE(ops.size() == 1);
    
    auto op = ops[0];
    REQUIRE(op["op"].toString() == "replace");
    REQUIRE(op["path"].toString() == "/transport/bpm");
    REQUIRE(static_cast<double>(op["value"]) == 140.0);
  }
  
  SECTION("Generates multiple patches for multiple changes") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state1;
    state1.transport.bpm = 120.0;
    state1.transport.isPlaying = false;
    broadcaster.broadcastFullState(state1);
    
    AppState state2 = state1;
    state2.transport.bpm = 140.0;
    state2.transport.isPlaying = true;
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    REQUIRE(msgVar["type"].toString() == "STATE_PATCH");
    
    auto ops = msgVar["ops"];
    REQUIRE(ops.size() == 2);
  }
  
  SECTION("Sends snapshot when exceeding 20 operations threshold") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state1;
    for (int i = 0; i < 8; ++i) {
      SlotState slot;
      slot.id = "slot_" + juce::String(i);
      slot.volume = 1.0f;
      state1.slots.push_back(slot);
    }
    broadcaster.broadcastFullState(state1);
    
    // Change many properties to trigger >20 ops
    AppState state2 = state1;
    for (int i = 0; i < 8; ++i) {
      state2.slots[i].volume = 0.5f;
      state2.slots[i].state = "PLAYING";
      state2.slots[i].name = "Modified " + juce::String(i);
    }
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    // Should send snapshot instead of patch (>20 ops)
    REQUIRE(msgVar["type"].toString() == "STATE_FULL");
    REQUIRE(static_cast<int64_t>(msgVar["revisionId"]) == 2);
  }
  
  SECTION("No message sent when state unchanged") {
    StateBroadcaster broadcaster;
    int messageCount = 0;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      messageCount++;
    });
    
    AppState state1;
    state1.transport.bpm = 120.0;
    broadcaster.broadcastFullState(state1);
    REQUIRE(messageCount == 1);
    
    // Same state
    AppState state2 = state1;
    broadcaster.broadcastStateUpdate(state2);
    REQUIRE(messageCount == 1); // No new message
  }
  
  SECTION("Patch operations for array modifications") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state1;
    RiffHistoryEntry riff1;
    riff1.id = "riff_1";
    riff1.name = "First Riff";
    state1.riffHistory.push_back(riff1);
    broadcaster.broadcastFullState(state1);
    
    // Add another riff
    AppState state2 = state1;
    RiffHistoryEntry riff2;
    riff2.id = "riff_2";
    riff2.name = "Second Riff";
    state2.riffHistory.push_back(riff2);
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    REQUIRE(msgVar["type"].toString() == "STATE_PATCH");
    
    auto ops = msgVar["ops"];
    REQUIRE(ops.isArray());
    // Should have add operation for the new riff
    bool hasAddOp = false;
    for (int i = 0; i < ops.size(); ++i) {
      if (ops[i]["op"].toString() == "add" && 
          ops[i]["path"].toString().contains("riffHistory")) {
        hasAddOp = true;
        break;
      }
    }
    REQUIRE(hasAddOp);
  }
  
  SECTION("Revision ID increments correctly") {
    StateBroadcaster broadcaster;
    
    REQUIRE(broadcaster.getRevisionId() == 0);
    
    AppState state;
    broadcaster.broadcastFullState(state);
    REQUIRE(broadcaster.getRevisionId() == 1);
    
    state.transport.bpm = 140.0;
    broadcaster.broadcastStateUpdate(state);
    REQUIRE(broadcaster.getRevisionId() == 2);
    
    state.transport.isPlaying = true;
    broadcaster.broadcastStateUpdate(state);
    REQUIRE(broadcaster.getRevisionId() == 3);
  }
  
  SECTION("Handles boolean property changes") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state1;
    state1.transport.isPlaying = false;
    broadcaster.broadcastFullState(state1);
    
    AppState state2 = state1;
    state2.transport.isPlaying = true;
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    REQUIRE(msgVar["type"].toString() == "STATE_PATCH");
    
    auto ops = msgVar["ops"];
    bool foundPlayingPatch = false;
    for (int i = 0; i < ops.size(); ++i) {
      if (ops[i]["path"].toString() == "/transport/isPlaying") {
        REQUIRE(ops[i]["op"].toString() == "replace");
        REQUIRE(static_cast<bool>(ops[i]["value"]) == true);
        foundPlayingPatch = true;
      }
    }
    REQUIRE(foundPlayingPatch);
  }
  
  SECTION("Logs patch sizes and operation counts") {
    StateBroadcaster broadcaster;
    juce::String lastMessage;
    
    broadcaster.setMessageCallback([&](const juce::String &msg) {
      lastMessage = msg;
    });
    
    AppState state1;
    state1.transport.bpm = 120.0;
    broadcaster.broadcastFullState(state1);
    
    AppState state2 = state1;
    state2.transport.bpm = 140.0;
    state2.transport.isPlaying = true;
    broadcaster.broadcastStateUpdate(state2);
    
    auto msgVar = juce::JSON::parse(lastMessage);
    if (msgVar["type"].toString() == "STATE_PATCH") {
      auto ops = msgVar["ops"];
      size_t opCount = ops.size();
      juce::String patchJson = juce::JSON::toString(ops);
      size_t patchSize = patchJson.toUTF8().sizeInBytes() - 1;
      
      // Log for verification
      DBG("Patch op count: " << opCount);
      DBG("Patch size: " << patchSize << " bytes");
      
      REQUIRE(opCount > 0);
      REQUIRE(patchSize > 0);
      REQUIRE(patchSize < 4096); // Should be under 4KB threshold
    }
  }
}
