#pragma once
#include <JuceHeader.h>
#include <chrono>
#include <deque>
#include <map>

namespace flowzone {

/**
 * CrashGuard: Three-tier crash recovery system
 * 
 * Level 1: Plugin crash loop (same manufacturer 3x in 60s) → disable VSTs only
 * Level 2: Audio driver failure → disable VSTs + reset audio device
 * Level 3: Crash counter >= 3 → factory defaults offered
 * 
 * Sentinel file lifecycle: create → increment → clear after 60s stable
 */
class CrashGuard {
public:
  enum class SafeMode {
    None = 0,
    Level1_DisableVSTs = 1,     // Plugin crash loop detected
    Level2_ResetAudio = 2,       // Audio driver failure
    Level3_FactoryDefaults = 3   // Repeated crashes
  };

  struct CrashEvent {
    std::chrono::system_clock::time_point timestamp;
    juce::String pluginManufacturer; // Empty if not plugin-related
    SafeMode level;
    juce::String reason;
  };

  CrashGuard() {
    sentinelFile =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("FlowZone")
            .getChildFile("CrashGuard_Sentinel.json");
    
    logFile =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("FlowZone")
            .getChildFile("CrashGuard_Log.txt");
    
    loadState();
  }

  ~CrashGuard() {
    saveState();
  }

  /**
   * Mark application as active. Call at startup.
   */
  void markActive() {
    auto now = std::chrono::system_clock::now();
    
    // Check if sentinel exists (indicates previous crash)
    if (sentinelFile.exists()) {
      recordCrash("Application did not exit cleanly", "", SafeMode::None);
    }
    
    // Create new sentinel
    sentinelFile.create();
    lastActiveTime = now;
    saveState();
    
    logStateTransition("Application started");
  }

  /**
   * Mark application as exiting cleanly.
   */
  void markClean() {
    purgeOldCrashes();
    
    if (sentinelFile.exists()) {
      sentinelFile.deleteFile();
      logStateTransition("Application exited cleanly");
    }
    
    // If stable for 60+ seconds, clear crash history
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastActiveTime).count();
    
    if (elapsed >= 60) {
      crashHistory.clear();
      pluginCrashCounts.clear();
      saveState();
      logStateTransition("Crash history cleared (stable 60s)");
    }
  }

  /**
   * Check if previous run crashed.
   */
  bool wasCrashed() const { 
    return sentinelFile.exists(); 
  }

  /**
   * Record a plugin crash.
   */
  void recordPluginCrash(const juce::String& manufacturer, const juce::String& pluginName) {
    juce::String reason = "Plugin crash: " + manufacturer + " - " + pluginName;
    
    // Track manufacturer crash count
    pluginCrashCounts[manufacturer]++;
    
    // Check for crash loop (3 crashes from same manufacturer in 60s)
    int recentCrashes = countRecentPluginCrashes(manufacturer, 60);
    
    SafeMode level = SafeMode::None;
    if (recentCrashes >= 3) {
      level = SafeMode::Level1_DisableVSTs;
      reason += " (CRASH LOOP DETECTED - disabling VSTs)";
    }
    
    recordCrash(reason, manufacturer, level);
  }

  /**
   * Record an audio driver failure.
   */
  void recordAudioFailure(const juce::String& deviceName) {
    juce::String reason = "Audio driver failure: " + deviceName;
    recordCrash(reason, "", SafeMode::Level2_ResetAudio);
  }

  /**
   * Get current safe mode level.
   */
  SafeMode getSafeModeLevel() const {
    purgeOldCrashes();
    
    // Check total crash count (Level 3)
    if (crashHistory.size() >= 3) {
      return SafeMode::Level3_FactoryDefaults;
    }
    
    // Check for Level 2 (audio failure)
    for (const auto& crash : crashHistory) {
      if (crash.level == SafeMode::Level2_ResetAudio) {
        return SafeMode::Level2_ResetAudio;
      }
    }
    
    // Check for Level 1 (plugin crash loop)
    for (const auto& [manufacturer, count] : pluginCrashCounts) {
      if (count >= 3) {
        return SafeMode::Level1_DisableVSTs;
      }
    }
    
    return SafeMode::None;
  }

  /**
   * Get safe mode description.
   */
  juce::String getSafeModeDescription() const {
    switch (getSafeModeLevel()) {
      case SafeMode::Level1_DisableVSTs:
        return "Safe Mode Level 1: VST plugins disabled due to crash loop";
      case SafeMode::Level2_ResetAudio:
        return "Safe Mode Level 2: Audio device reset due to driver failure";
      case SafeMode::Level3_FactoryDefaults:
        return "Safe Mode Level 3: Factory defaults recommended (repeated crashes)";
      case SafeMode::None:
      default:
        return "Normal operation";
    }
  }

  /**
   * Check if VSTs should be disabled.
   */
  bool shouldDisableVSTs() const {
    auto level = getSafeModeLevel();
    return level >= SafeMode::Level1_DisableVSTs;
  }

  /**
   * Check if audio device should be reset.
   */
  bool shouldResetAudio() const {
    auto level = getSafeModeLevel();
    return level >= SafeMode::Level2_ResetAudio;
  }

  /**
   * Check if factory defaults should be offered.
   */
  bool shouldOfferFactoryDefaults() const {
    return getSafeModeLevel() == SafeMode::Level3_FactoryDefaults;
  }

  /**
   * Clear crash history (user confirmed recovery).
   */
  void clearCrashHistory() {
    crashHistory.clear();
    pluginCrashCounts.clear();
    
    if (sentinelFile.exists()) {
      sentinelFile.deleteFile();
    }
    
    saveState();
    logStateTransition("Crash history cleared by user");
  }

  /**
   * Get recent crash events for display.
   */
  std::vector<CrashEvent> getRecentCrashes(int maxAge = 300) const {
    purgeOldCrashes();
    
    auto now = std::chrono::system_clock::now();
    std::vector<CrashEvent> recent;
    
    for (const auto& crash : crashHistory) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - crash.timestamp).count();
      
      if (elapsed <= maxAge) {
        recent.push_back(crash);
      }
    }
    
    return recent;
  }

private:
  juce::File sentinelFile;
  juce::File logFile;
  std::chrono::system_clock::time_point lastActiveTime;
  
  mutable std::deque<CrashEvent> crashHistory;
  std::map<juce::String, int> pluginCrashCounts;

  void recordCrash(const juce::String& reason, 
                   const juce::String& manufacturer,
                   SafeMode level) {
    CrashEvent event;
    event.timestamp = std::chrono::system_clock::now();
    event.pluginManufacturer = manufacturer;
    event.level = level;
    event.reason = reason;
    
    crashHistory.push_back(event);
    
    // Keep only last 10 crashes
    if (crashHistory.size() > 10) {
      crashHistory.pop_front();
    }
    
    saveState();
    logStateTransition("CRASH: " + reason);
  }

  int countRecentPluginCrashes(const juce::String& manufacturer, int seconds) const {
    auto now = std::chrono::system_clock::now();
    int count = 0;
    
    for (const auto& crash : crashHistory) {
      if (crash.pluginManufacturer == manufacturer) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - crash.timestamp).count();
        
        if (elapsed <= seconds) {
          count++;
        }
      }
    }
    
    return count;
  }

  void purgeOldCrashes() const {
    auto now = std::chrono::system_clock::now();
    
    // Remove crashes older than 5 minutes
    while (!crashHistory.empty()) {
      auto& oldest = crashHistory.front();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - oldest.timestamp).count();
      
      if (elapsed > 300) {
        crashHistory.pop_front();
      } else {
        break;
      }
    }
  }

  void loadState() {
    if (!sentinelFile.exists()) {
      return;
    }
    
    try {
      auto json = juce::JSON::parse(sentinelFile);
      if (json.isObject()) {
        auto crashArray = json["crashes"];
        if (crashArray.isArray()) {
          for (const auto& item : *crashArray.getArray()) {
            CrashEvent event;
            event.timestamp = std::chrono::system_clock::from_time_t(
                static_cast<time_t>(item["timestamp"].operator int64()));
            event.pluginManufacturer = item["manufacturer"].toString();
            event.level = static_cast<SafeMode>(item["level"].operator int());
            event.reason = item["reason"].toString();
            
            crashHistory.push_back(event);
            
            if (!event.pluginManufacturer.isEmpty()) {
              pluginCrashCounts[event.pluginManufacturer]++;
            }
          }
        }
      }
    } catch (...) {
      // Corrupt state file - ignore
      crashHistory.clear();
      pluginCrashCounts.clear();
    }
  }

  void saveState() {
    sentinelFile.getParentDirectory().createDirectory();
    
    juce::var::array crashArray;
    for (const auto& crash : crashHistory) {
      auto crashObj = new juce::DynamicObject();
      crashObj->setProperty("timestamp", 
          static_cast<int64>(std::chrono::system_clock::to_time_t(crash.timestamp)));
      crashObj->setProperty("manufacturer", crash.pluginManufacturer);
      crashObj->setProperty("level", static_cast<int>(crash.level));
      crashObj->setProperty("reason", crash.reason);
      
      crashArray.add(juce::var(crashObj));
    }
    
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("crashes", crashArray);
    
    auto jsonText = juce::JSON::toString(juce::var(root.get()), true);
    sentinelFile.replaceWithText(jsonText);
  }

  void logStateTransition(const juce::String& message) const {
    logFile.getParentDirectory().createDirectory();
    
    auto now = juce::Time::getCurrentTime();
    auto timestamp = now.toISO8601(true);
    
    juce::String logEntry = timestamp + " | " + message + "\n";
    
    // Append to log file
    if (logFile.existsAsFile()) {
      auto existingContent = logFile.loadFileAsString();
      logFile.replaceWithText(existingContent + logEntry);
    } else {
      logFile.replaceWithText(logEntry);
    }
    
    // Also log to console
    DBG("CrashGuard: " << message);
  }
};

} // namespace flowzone
