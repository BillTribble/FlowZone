#include "ConfigManager.h"

namespace flowzone {

ConfigManager::ConfigManager() {
  auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
      .getChildFile("FlowZone");
  
  appDataDir.createDirectory();
  
  configFile = appDataDir.getChildFile("config.json");
  backupFile = appDataDir.getChildFile("state.backup.1.json");
  legacyFile = appDataDir.getChildFile("config.legacy.json");
}

ConfigManager::Config ConfigManager::loadConfig() {
  logEvent("Loading configuration...");
  
  // Step 1: Try to load main config file
  bool success = false;
  Config config = loadFromFile(configFile, success);
  
  if (!success) {
    logEvent("Failed to load config.json, trying backup...");
    
    // Step 2: Try backup
    config = loadFromFile(backupFile, success);
    
    if (!success) {
      logEvent("Backup failed, loading factory defaults...");
      
      // Step 3: Factory defaults
      config = getFactoryDefaults();
      
      // Archive any corrupt config as legacy
      if (configFile.existsAsFile()) {
        configFile.copyFileTo(legacyFile);
        logEvent("Archived corrupt config as config.legacy.json");
      }
    } else {
      logEvent("Loaded from backup successfully");
    }
  }
  
  // Step 4: Check version and migrate if needed
  if (config.configVersion < CURRENT_CONFIG_VERSION) {
    logEvent(juce::String("Migrating config from v") + 
             juce::String(config.configVersion) + " to v" + 
             juce::String(CURRENT_CONFIG_VERSION));
    
    bool migrated = migrateConfig(config, config.configVersion, CURRENT_CONFIG_VERSION);
    
    if (!migrated) {
      logEvent("Migration failed, using factory defaults");
      
      // Archive the old config
      if (configFile.existsAsFile()) {
        configFile.copyFileTo(legacyFile);
        logEvent("Archived old config as config.legacy.json");
      }
      
      config = getFactoryDefaults();
    } else {
      logEvent("Migration successful");
    }
  }
  
  // Step 5: Validate final config
  if (config.sampleRate <= 0 || config.bufferSize <= 0) {
    logEvent("Invalid config values, using minimum safe config");
    config = getMinimumSafeConfig();
  }
  
  // Save the loaded/migrated config
  saveConfig(config);
  
  return config;
}

bool ConfigManager::saveConfig(const Config& config) {
  try {
    // Create backup of current config before overwriting
    if (configFile.existsAsFile()) {
      createBackup(configFile);
    }
    
    // Write new config
    auto jsonVar = config.toVar();
    juce::String jsonText = juce::JSON::toString(jsonVar, true);
    
    bool written = configFile.replaceWithText(jsonText);
    
    if (written) {
      logEvent("Config saved successfully");
      return true;
    } else {
      logEvent("Failed to write config file");
      return false;
    }
  } catch (const std::exception& e) {
    logEvent(juce::String("Exception saving config: ") + e.what());
    return false;
  } catch (...) {
    logEvent("Unknown exception saving config");
    return false;
  }
}

ConfigManager::Config ConfigManager::getFactoryDefaults() {
  Config config;
  config.configVersion = CURRENT_CONFIG_VERSION;
  config.sampleRate = 44100.0;
  config.bufferSize = 512;
  config.enablePlugins = true;
  config.audioDeviceName = "";
  config.disabledPlugins.clear();
  
  return config;
}

ConfigManager::Config ConfigManager::getMinimumSafeConfig() {
  Config config;
  config.configVersion = CURRENT_CONFIG_VERSION;
  config.sampleRate = 44100.0;
  config.bufferSize = 512;
  config.enablePlugins = false; // Disable plugins in safe mode
  config.audioDeviceName = "";
  config.disabledPlugins.clear();
  
  return config;
}

ConfigManager::Config ConfigManager::loadFromFile(const juce::File& file, bool& success) {
  success = false;
  Config config;
  
  if (!file.existsAsFile()) {
    logEvent("Config file does not exist: " + file.getFullPathName());
    return config;
  }
  
  try {
    juce::String jsonText = file.loadFileAsString();
    
    if (parseAndValidate(jsonText, config)) {
      success = true;
      logEvent("Loaded config from: " + file.getFileName());
    } else {
      logEvent("Failed to parse config from: " + file.getFileName());
    }
  } catch (const std::exception& e) {
    logEvent(juce::String("Exception loading config: ") + e.what());
  } catch (...) {
    logEvent("Unknown exception loading config");
  }
  
  return config;
}

bool ConfigManager::parseAndValidate(const juce::String& jsonText, Config& outConfig) {
  try {
    auto jsonVar = juce::JSON::parse(jsonText);
    
    if (!jsonVar.isObject()) {
      logEvent("JSON is not an object");
      return false;
    }
    
    outConfig = Config::fromVar(jsonVar);
    
    // Basic validation
    if (outConfig.sampleRate <= 0) {
      logEvent("Invalid sample rate in config");
      return false;
    }
    
    if (outConfig.bufferSize <= 0 || outConfig.bufferSize > 8192) {
      logEvent("Invalid buffer size in config");
      return false;
    }
    
    return true;
  } catch (...) {
    logEvent("Exception parsing JSON");
    return false;
  }
}

bool ConfigManager::migrateConfig(Config& config, int fromVersion, int toVersion) {
  if (fromVersion == toVersion) {
    return true; // No migration needed
  }
  
  // Migrate step by step
  int currentVersion = fromVersion;
  
  while (currentVersion < toVersion) {
    bool success = false;
    
    if (currentVersion == 0 && toVersion >= 1) {
      success = migrateV0toV1(config);
      if (success) {
        currentVersion = 1;
      }
    }
    // Add more migration paths here as versions increase
    
    if (!success) {
      logEvent(juce::String("Migration failed at version ") + juce::String(currentVersion));
      return false;
    }
  }
  
  config.configVersion = toVersion;
  logEvent(juce::String("Migration complete: v") + juce::String(fromVersion) + 
           " → v" + juce::String(toVersion));
  
  return true;
}

bool ConfigManager::migrateV0toV1(Config& config) {
  logEvent("Migrating v0 → v1");
  
  // V0 didn't have configVersion field
  // V1 adds: configVersion, enablePlugins, disabledPlugins
  
  // If migrating from v0, assume enablePlugins should be true
  // (unless we're being conservative)
  config.enablePlugins = true;
  config.disabledPlugins.clear();
  
  // Ensure other fields have valid values
  if (config.sampleRate <= 0) {
    config.sampleRate = 44100.0;
  }
  
  if (config.bufferSize <= 0) {
    config.bufferSize = 512;
  }
  
  return true;
}

void ConfigManager::createBackup(const juce::File& source) {
  try {
    if (!source.existsAsFile()) {
      return;
    }
    
    // Copy to backup
    source.copyFileTo(backupFile);
    logEvent("Backup created: " + backupFile.getFileName());
  } catch (...) {
    logEvent("Failed to create backup");
  }
}

void ConfigManager::logEvent(const juce::String& message) {
  auto timestamp = juce::Time::getCurrentTime().toISO8601(true);
  juce::String logMessage = timestamp + " | ConfigManager: " + message;
  
  DBG(logMessage);
  
  // Call event callback if set
  if (onConfigEvent) {
    juce::MessageManager::callAsync([this, message]() {
      if (onConfigEvent) {
        onConfigEvent(message);
      }
    });
  }
  
  // Also write to log file
  auto logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
      .getChildFile("FlowZone")
      .getChildFile("ConfigManager_Log.txt");
  
  try {
    if (logFile.existsAsFile()) {
      auto existing = logFile.loadFileAsString();
      logFile.replaceWithText(existing + logMessage + "\n");
    } else {
      logFile.replaceWithText(logMessage + "\n");
    }
  } catch (...) {
    // Silent fail on log write
  }
}

} // namespace flowzone
