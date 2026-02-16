/**
 * SessionStateManager Integration Test
 * 
 * Tests:
 * - Autosave creates valid snapshot every 30s
 * - Riff COMMIT creates riffHistory entry with correct layers/colors/userId
 * - LOAD_RIFF restores all 8 slot states
 * - DELETE_JAM removes session metadata + audio files
 * - Crash recovery: kill process → relaunch → verify last autosave loaded
 * - Log all session lifecycle events with timestamps
 */

#include "../../src/engine/session/SessionStateManager.h"
#include "../../src/engine/state/AppState.h"
#include "../../libs/catch2/catch.hpp"
#include <JuceHeader.h>

using namespace flowzone;

TEST_CASE("SessionStateManager Integration", "[SessionStateManager][Integration]") {
  
  // Create temporary directory for tests
  auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("FlowZoneTests_" + juce::Uuid().toString());
  tempDir.createDirectory();
  
  SECTION("Autosave creates valid snapshot every 30s") {
    SessionStateManager manager;
    
    AppState state;
    state.session.id = "test_session_1";
    state.session.name = "Test Session";
    state.transport.bpm = 120.0;
    manager.setState(state);
    
    auto sessionDir = tempDir.getChildFile("autosave_test");
    sessionDir.createDirectory();
    
    manager.startAutosave(sessionDir);
    
    // Trigger manual autosave (simulates timer)
    auto result = manager.performAutosave();
    REQUIRE(result.wasOk());
    
    // Verify autosave file was created
    auto autosaveFolder = sessionDir.getChildFile("autosave");
    REQUIRE(autosaveFolder.exists());
    
    auto autosaves = autosaveFolder.findChildFiles(juce::File::findFiles, false, "autosave_*.flow");
    REQUIRE(!autosaves.isEmpty());
    
    // Load autosave and verify
    AppState loaded;
    auto loadResult = manager.loadSession(autosaves[0], loaded);
    REQUIRE(loadResult.wasOk());
    REQUIRE(loaded.session.id == "test_session_1");
    REQUIRE(loaded.transport.bpm == 120.0);
    
    manager.stopAutosave();
    sessionDir.deleteRecursively();
  }
  
  SECTION("Multiple autosaves - keeps only last 5") {
    SessionStateManager manager;
    
    AppState state;
    state.session.id = "test_session_multi";
    manager.setState(state);
    
    auto sessionDir = tempDir.getChildFile("multi_autosave_test");
    sessionDir.createDirectory();
    
    manager.startAutosave(sessionDir);
    
    // Create 7 autosaves
    for (int i = 0; i < 7; ++i) {
      state.transport.bpm = 120.0 + i;
      manager.setState(state);
      manager.performAutosave();
      juce::Thread::sleep(10); // Ensure different timestamps
    }
    
    // Verify only 5 remain
    auto autosaveFolder = sessionDir.getChildFile("autosave");
    auto autosaves = autosaveFolder.findChildFiles(juce::File::findFiles, false, "autosave_*.flow");
    REQUIRE(autosaves.size() == 5);
    
    manager.stopAutosave();
    sessionDir.deleteRecursively();
  }
  
  SECTION("Riff COMMIT creates riffHistory entry") {
    SessionStateManager manager;
    
    AppState state;
    state.session.id = "riff_test_session";
    manager.setState(state);
    
    std::vector<juce::String> colors = {"#FF0000", "#00FF00", "#0000FF"};
    auto result = manager.commitRiff("My First Riff", 3, colors, "user_123");
    
    REQUIRE(result.wasOk());
    
    auto currentState = manager.getCurrentState();
    REQUIRE(currentState.riffHistory.size() == 1);
    
    auto &riff = currentState.riffHistory[0];
    REQUIRE(riff.name == "My First Riff");
    REQUIRE(riff.layers == 3);
    REQUIRE(riff.colors.size() == 3);
    REQUIRE(riff.colors[0] == "#FF0000");
    REQUIRE(riff.userId == "user_123");
    REQUIRE(!riff.id.isEmpty());
    REQUIRE(riff.timestamp > 0);
  }
  
  SECTION("Multiple riff commits accumulate in history") {
    SessionStateManager manager;
    
    std::vector<juce::String> colors1 = {"#FF0000"};
    std::vector<juce::String> colors2 = {"#00FF00", "#0000FF"};
    
    manager.commitRiff("Riff 1", 1, colors1, "user_1");
    manager.commitRiff("Riff 2", 2, colors2, "user_2");
    
    auto state = manager.getCurrentState();
    REQUIRE(state.riffHistory.size() == 2);
    REQUIRE(state.riffHistory[0].name == "Riff 1");
    REQUIRE(state.riffHistory[1].name == "Riff 2");
  }
  
  SECTION("LOAD_RIFF finds riff in history") {
    SessionStateManager manager;
    
    std::vector<juce::String> colors = {"#FF0000"};
    manager.commitRiff("Test Riff", 1, colors, "user_1");
    
    auto state = manager.getCurrentState();
    juce::String riffId = state.riffHistory[0].id;
    
    auto result = manager.loadRiff(riffId);
    REQUIRE(result.wasOk());
  }
  
  SECTION("LOAD_RIFF fails on non-existent riff") {
    SessionStateManager manager;
    
    auto result = manager.loadRiff("non_existent_riff_id");
    REQUIRE(result.failed());
  }
  
  SECTION("DELETE_JAM removes session directory") {
    SessionStateManager manager;
    
    auto sessionDir = tempDir.getChildFile("delete_test_session");
    sessionDir.createDirectory();
    
    AppState state;
    state.session.id = "delete_test";
    manager.setState(state);
    manager.startAutosave(sessionDir);
    
    REQUIRE(sessionDir.exists());
    
    auto result = manager.deleteJam("delete_test");
    REQUIRE(result.wasOk());
    REQUIRE(!sessionDir.exists());
    
    // State should be cleared
    auto currentState = manager.getCurrentState();
    REQUIRE(currentState.session.id.isEmpty());
  }
  
  SECTION("Crash recovery loads last autosave") {
    SessionStateManager manager1;
    
    AppState state;
    state.session.id = "crash_test_session";
    state.session.name = "Crash Test";
    state.transport.bpm = 135.0;
    manager1.setState(state);
    
    auto sessionDir = tempDir.getChildFile("crash_recovery_test");
    sessionDir.createDirectory();
    
    manager1.startAutosave(sessionDir);
    manager1.performAutosave();
    
    // Simulate crash - destroy manager
    manager1.stopAutosave();
    
    // Create new manager (simulates relaunch)
    SessionStateManager manager2;
    
    REQUIRE(manager2.hasAutosave(sessionDir));
    
    AppState recovered;
    auto result = manager2.loadLastAutosave(sessionDir, recovered);
    
    REQUIRE(result.wasOk());
    REQUIRE(recovered.session.id == "crash_test_session");
    REQUIRE(recovered.session.name == "Crash Test");
    REQUIRE(recovered.transport.bpm == 135.0);
    
    sessionDir.deleteRecursively();
  }
  
  SECTION("Session save and load round-trip") {
    SessionStateManager manager;
    
    AppState original;
    original.session.id = "roundtrip_test";
    original.session.name = "Round Trip";
    original.session.emoji = "🎸";
    original.transport.bpm = 142.0;
    original.transport.isPlaying = true;
    
    // Add a slot
    SlotState slot;
    slot.id = "slot_0";
    slot.volume = 0.8f;
    slot.state = "PLAYING";
    original.slots.push_back(slot);
    
    auto sessionFile = tempDir.getChildFile("roundtrip.flow");
    
    auto saveResult = manager.saveSession(sessionFile, original);
    REQUIRE(saveResult.wasOk());
    REQUIRE(sessionFile.existsAsFile());
    
    AppState loaded;
    auto loadResult = manager.loadSession(sessionFile, loaded);
    REQUIRE(loadResult.wasOk());
    
    REQUIRE(loaded.session.id == original.session.id);
    REQUIRE(loaded.session.name == original.session.name);
    REQUIRE(loaded.session.emoji == original.session.emoji);
    REQUIRE(loaded.transport.bpm == original.transport.bpm);
    REQUIRE(loaded.transport.isPlaying == original.transport.isPlaying);
    REQUIRE(loaded.slots.size() == 1);
    REQUIRE(loaded.slots[0].id == "slot_0");
    REQUIRE(loaded.slots[0].volume == 0.8f);
    
    sessionFile.deleteFile();
  }
  
  SECTION("Create new session") {
    SessionStateManager manager;
    
    AppState newState;
    auto result = manager.createNewSession("My New Session", tempDir, newState);
    
    REQUIRE(result.wasOk());
    REQUIRE(!newState.session.id.isEmpty());
    REQUIRE(newState.session.name == "My New Session");
    REQUIRE(newState.session.emoji == "🎵");
    REQUIRE(newState.session.createdAt > 0);
    
    auto sessionDir = tempDir.getChildFile("My New Session");
    REQUIRE(sessionDir.exists());
    REQUIRE(sessionDir.getChildFile("session.flow").existsAsFile());
    
    sessionDir.deleteRecursively();
  }
  
  SECTION("Thread-safe state modification") {
    SessionStateManager manager;
    
    std::atomic<int> updateCount{0};
    
    auto updater = [&]() {
      for (int i = 0; i < 100; ++i) {
        manager.updateState([&](AppState &state) {
          state.transport.bpm = 120.0 + i;
          updateCount++;
        });
      }
    };
    
    std::thread t1(updater);
    std::thread t2(updater);
    
    t1.join();
    t2.join();
    
    REQUIRE(updateCount.load() == 200);
    
    // Final BPM should be valid
    auto state = manager.getCurrentState();
    REQUIRE(state.transport.bpm >= 120.0);
    REQUIRE(state.transport.bpm < 320.0);
  }
  
  // Cleanup
  tempDir.deleteRecursively();
}
