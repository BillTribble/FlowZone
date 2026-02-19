#pragma once

#include <JuceHeader.h>

namespace flowzone {

/**
 * ConfigManager: Versioned configuration with migration and fallback
 * 
 * Load sequence:
 * 1. Parse config.json (fail → backup)
 * 2. Check version (old → migration)
 * 3. Migration fail → factory defaults + archive as config.legacy.json
 * 4. Factory fail → hardcoded minimum config
 * 
 * Backup: state.backup.1.json maintained alongside config.json
 */
class ConfigManager {
public:
  static constexpr int CURRENT_CONFIG_VERSION = 1;
  
  struct Config {
    int configVersion = CURRENT_CONFIG_VERSION;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    bool enablePlugins = true;
    juce::String audioDeviceName;
    juce::StringArray disabledPlugins;
    
    juce::var toVar() const {
      auto obj = new juce::DynamicObject();
      obj->setProperty("configVersion", configVersion);
      obj->setProperty("sampleRate", sampleRate);
      obj->setProperty("bufferSize", bufferSize);
      obj->setProperty("enablePlugins", enablePlugins);
      obj->setProperty("audioDeviceName", audioDeviceName);
      
      juce::Array<juce::var> plugins;
      for (const auto& plugin : disabledPlugins) {
        plugins.add(plugin);
      }
      obj->setProperty("disabledPlugins", plugins);
      
      return juce::var(obj);
    }
    
    static Config fromVar(const juce::var& v) {
      Config config;
      
      if (!v.isObject()) {
        return config;
      }
      
      auto obj = v.getDynamicObject();
      if (!obj) {
        return config;
      }
      
      config.configVersion = obj->getProperty("configVersion");
      config.sampleRate = obj->getProperty("sampleRate");
      config.bufferSize = obj->getProperty("bufferSize");
      config.enablePlugins = obj->getProperty("enablePlugins");
      config.audioDeviceName = obj->getProperty("audioDeviceName").toString();
      
      auto pluginsArray = obj->getProperty("disabledPlugins");
      if (pluginsArray.isArray()) {
        for (const auto& plugin : *pluginsArray.getArray()) {
          config.disabledPlugins.add(plugin.toString());
        }
      }
      
      return config;
    }
  };

  ConfigManager();
  
  /**
   * Load configuration with full fallback chain.
   */
  Config loadConfig();
  
  /**
   * Save configuration with backup.
   */
  bool saveConfig(const Config& config);
  
  /**
   * Get factory default configuration.
   */
  static Config getFactoryDefaults();
  
  /**
   * Get hardcoded minimum safe configuration.
   */
  static Config getMinimumSafeConfig();
  
  /**
   * Set callback for migration/load events.
   */
  std::function<void(const juce::String&)> onConfigEvent;

private:
  juce::File configFile;
  juce::File backupFile;
  juce::File legacyFile;
  
  // Load helpers
  Config loadFromFile(const juce::File& file, bool& success);
  bool parseAndValidate(const juce::String& jsonText, Config& outConfig);
  
  // Migration
  bool migrateConfig(Config& config, int fromVersion, int toVersion);
  bool migrateV0toV1(Config& config);
  
  // Backup
  void createBackup(const juce::File& source);
  
  // Logging
  void logEvent(const juce::String& message);
};

} // namespace flowzone
