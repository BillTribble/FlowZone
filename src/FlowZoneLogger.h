#pragma once
#include <juce_core/juce_core.h>

/**
 * Dedicated logger for FlowZone V9 audit logging.
 * Logs user actions and audio performance stats to ~/FlowZone_Log.txt.
 */
class FlowZoneLogger {
public:
  static void logStartup() { log("SYSTEM", "Application Startup"); }

  static void logAction(const juce::String &tag, const juce::String &data) {
    log("ACTION::" + tag, data);
  }

  static void logAudioStats(float peak, bool clipped) {
    if (clipped) {
      log("AUDIO", "CLIPPING DETECTED! Peak: " + juce::String(peak, 2));
    } else {
      // Only log peaks if they are significant or periodically
      if (peak > 0.8f) {
        log("AUDIO", "High Peak: " + juce::String(peak, 2));
      }
    }
  }

private:
  static void log(const juce::String &category, const juce::String &message) {
    juce::String logLine = "[" +
                           juce::Time::getCurrentTime().toString(true, true) +
                           "] " + "[" + category + "] " + message;
    juce::Logger::writeToLog(logLine);
  }
};

// Macros for easier usage
#define LOG_STARTUP() FlowZoneLogger::logStartup()
#define LOG_ACTION(tag, data) FlowZoneLogger::logAction(tag, data)
#define LOG_AUDIO_STATS(peak, clipped)                                         \
  FlowZoneLogger::logAudioStats(peak, clipped)
