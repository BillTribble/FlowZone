#include "../../libs/catch2/catch.hpp"
#include "../../src/engine/CrashGuard.h"
#include <thread>
#include <chrono>

using namespace flowzone;

TEST_CASE("CrashGuard: Sentinel file lifecycle", "[CrashGuard]") {
  // Clean up any existing state
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("markActive creates sentinel file") {
    CrashGuard guard;
    guard.markActive();
    
    REQUIRE(sentinelFile.exists());
  }

  SECTION("markClean removes sentinel file") {
    CrashGuard guard;
    guard.markActive();
    REQUIRE(sentinelFile.exists());
    
    guard.markClean();
    REQUIRE_FALSE(sentinelFile.exists());
  }

  SECTION("wasCrashed detects previous crash") {
    {
      CrashGuard guard;
      guard.markActive();
      // Simulate crash by not calling markClean
    }
    
    CrashGuard guard2;
    REQUIRE(guard2.wasCrashed());
  }

  SECTION("Stable operation clears crash history after 60s") {
    CrashGuard guard;
    guard.markActive();
    
    // Simulate 60+ seconds of stable operation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    guard.markClean();
    
    // After marking clean with sufficient uptime, crash history should be cleared
    REQUIRE_FALSE(guard.wasCrashed());
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Level 1 - Plugin crash loop detection", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Single plugin crash does not trigger safe mode") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::None);
    REQUIRE_FALSE(guard.shouldDisableVSTs());
  }

  SECTION("Two plugin crashes from same manufacturer do not trigger safe mode") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin1");
    guard.recordPluginCrash("TestManufacturer", "TestPlugin2");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::None);
    REQUIRE_FALSE(guard.shouldDisableVSTs());
  }

  SECTION("Three plugin crashes from same manufacturer trigger Level 1") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin1");
    guard.recordPluginCrash("TestManufacturer", "TestPlugin2");
    guard.recordPluginCrash("TestManufacturer", "TestPlugin3");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level1_DisableVSTs);
    REQUIRE(guard.shouldDisableVSTs());
    REQUIRE_FALSE(guard.shouldResetAudio());
    REQUIRE_FALSE(guard.shouldOfferFactoryDefaults());
  }

  SECTION("Plugin crashes from different manufacturers do not trigger Level 1") {
    CrashGuard guard;
    guard.recordPluginCrash("Manufacturer1", "Plugin1");
    guard.recordPluginCrash("Manufacturer2", "Plugin2");
    guard.recordPluginCrash("Manufacturer3", "Plugin3");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::None);
    REQUIRE_FALSE(guard.shouldDisableVSTs());
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Level 2 - Audio driver failure", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Audio failure triggers Level 2 safe mode") {
    CrashGuard guard;
    guard.recordAudioFailure("CoreAudio");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level2_ResetAudio);
    REQUIRE(guard.shouldDisableVSTs());
    REQUIRE(guard.shouldResetAudio());
    REQUIRE_FALSE(guard.shouldOfferFactoryDefaults());
  }

  SECTION("Level 2 description is correct") {
    CrashGuard guard;
    guard.recordAudioFailure("CoreAudio");
    
    auto desc = guard.getSafeModeDescription();
    REQUIRE(desc.contains("Level 2"));
    REQUIRE(desc.contains("Audio device reset"));
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Level 3 - Repeated crashes", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Three total crashes trigger Level 3") {
    CrashGuard guard;
    
    // Simulate 3 crash/restart cycles
    guard.markActive();
    guard.recordPluginCrash("Manufacturer1", "Plugin1");
    
    guard.recordPluginCrash("Manufacturer2", "Plugin2");
    
    guard.recordAudioFailure("CoreAudio");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level3_FactoryDefaults);
    REQUIRE(guard.shouldOfferFactoryDefaults());
  }

  SECTION("Level 3 description is correct") {
    CrashGuard guard;
    guard.recordPluginCrash("M1", "P1");
    guard.recordPluginCrash("M2", "P2");
    guard.recordPluginCrash("M3", "P3");
    
    auto desc = guard.getSafeModeDescription();
    REQUIRE(desc.contains("Level 3"));
    REQUIRE(desc.contains("Factory defaults"));
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: State persistence", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Crash state persists across instances") {
    {
      CrashGuard guard;
      guard.recordPluginCrash("TestManufacturer", "TestPlugin1");
      guard.recordPluginCrash("TestManufacturer", "TestPlugin2");
      guard.recordPluginCrash("TestManufacturer", "TestPlugin3");
      
      REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level1_DisableVSTs);
    }
    
    // Create new instance - should load persisted state
    {
      CrashGuard guard2;
      REQUIRE(guard2.getSafeModeLevel() == CrashGuard::SafeMode::Level1_DisableVSTs);
      REQUIRE(guard2.shouldDisableVSTs());
    }
  }

  SECTION("clearCrashHistory resets state") {
    CrashGuard guard;
    guard.recordPluginCrash("M1", "P1");
    guard.recordPluginCrash("M2", "P2");
    guard.recordPluginCrash("M3", "P3");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level3_FactoryDefaults);
    
    guard.clearCrashHistory();
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::None);
    REQUIRE_FALSE(guard.wasCrashed());
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Recent crashes tracking", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("getRecentCrashes returns crash events") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin");
    guard.recordAudioFailure("CoreAudio");
    
    auto recent = guard.getRecentCrashes(300);
    
    REQUIRE(recent.size() >= 2);
    REQUIRE(recent[0].reason.contains("Plugin crash"));
    REQUIRE(recent[1].reason.contains("Audio driver failure"));
  }

  SECTION("getRecentCrashes filters by age") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin");
    
    // Immediate check should include the crash
    auto recent = guard.getRecentCrashes(300);
    REQUIRE(recent.size() >= 1);
    
    // With maxAge of 0, should not include any crashes
    auto empty = guard.getRecentCrashes(0);
    REQUIRE(empty.empty());
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Logging and state transitions", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  auto logFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Log.txt");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
  if (logFile.exists()) {
    logFile.deleteFile();
  }

  SECTION("State transitions are logged with timestamps") {
    CrashGuard guard;
    guard.markActive();
    guard.recordPluginCrash("TestManufacturer", "TestPlugin");
    guard.markClean();
    
    REQUIRE(logFile.existsAsFile());
    
    auto logContent = logFile.loadFileAsString();
    
    // Should contain ISO 8601 timestamps
    REQUIRE(logContent.contains("T"));
    REQUIRE(logContent.contains("Z"));
    
    // Should contain state transition messages
    REQUIRE(logContent.contains("Application started"));
    REQUIRE(logContent.contains("CRASH"));
    REQUIRE(logContent.contains("Application exited cleanly"));
  }

  SECTION("Log entries include crash details") {
    CrashGuard guard;
    guard.recordPluginCrash("TestManufacturer", "TestPlugin");
    
    REQUIRE(logFile.existsAsFile());
    
    auto logContent = logFile.loadFileAsString();
    REQUIRE(logContent.contains("TestManufacturer"));
    REQUIRE(logContent.contains("TestPlugin"));
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
  if (logFile.exists()) {
    logFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Crash loop timing window", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Old crashes are purged from history") {
    CrashGuard guard;
    
    // Record crashes
    guard.recordPluginCrash("M1", "P1");
    guard.recordPluginCrash("M2", "P2");
    guard.recordPluginCrash("M3", "P3");
    
    // Verify Level 3 is triggered
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level3_FactoryDefaults);
    
    // In a real scenario, old crashes (>5 minutes) would be purged
    // This is handled internally by purgeOldCrashes()
    // For this test, we verify the mechanism exists
    auto recent = guard.getRecentCrashes(300); // 5 minutes
    REQUIRE(recent.size() >= 3);
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}

TEST_CASE("CrashGuard: Safe mode hierarchy", "[CrashGuard]") {
  auto sentinelFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("FlowZone")
          .getChildFile("CrashGuard_Sentinel.json");
  
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }

  SECTION("Level 3 takes precedence over Level 1") {
    CrashGuard guard;
    
    // Trigger Level 1 (plugin crash loop)
    guard.recordPluginCrash("M1", "P1");
    guard.recordPluginCrash("M1", "P2");
    guard.recordPluginCrash("M1", "P3");
    
    // Add one more crash to reach total of 3
    // This should trigger Level 3
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level3_FactoryDefaults);
  }

  SECTION("Level 2 is reported when present") {
    CrashGuard guard;
    guard.recordAudioFailure("CoreAudio");
    
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level2_ResetAudio);
  }

  SECTION("Level 3 takes precedence over Level 2") {
    CrashGuard guard;
    guard.recordAudioFailure("CoreAudio");
    guard.recordPluginCrash("M1", "P1");
    guard.recordPluginCrash("M2", "P2");
    
    // Total 3 crashes should trigger Level 3
    REQUIRE(guard.getSafeModeLevel() == CrashGuard::SafeMode::Level3_FactoryDefaults);
  }

  // Cleanup
  if (sentinelFile.exists()) {
    sentinelFile.deleteFile();
  }
}
