#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <fstream>
#include <mutex>
#include <string>

namespace flowzone {

/**
 * @brief File-based logger for debugging audio flow and WebSocket communication.
 *
 * Writes timestamped entries to separate log files in ~/FlowZone_Logs/.
 * Designed to be concise (one line per event) and safe for audio thread use
 * via sampling (caller controls how often to log).
 */
class FileLogger {
public:
  enum class Category { WebSocket, AudioFlow, StateBroadcast };

  static FileLogger &instance() {
    static FileLogger inst;
    return inst;
  }

  void log(Category cat, const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &stream = getStream(cat);
    if (stream.is_open()) {
      stream << getTimestamp() << " " << message << "\n";
      stream.flush();
    }
  }

  // Convenience: log only every Nth call (for audio thread sampling)
  // Returns true if this call actually logged
  bool logSampled(Category cat, const std::string &message, int &counter,
                  int interval) {
    if (++counter >= interval) {
      counter = 0;
      log(cat, message);
      return true;
    }
    return false;
  }

private:
  FileLogger() {
    auto logDir = juce::File::getSpecialLocation(
                      juce::File::userHomeDirectory)
                      .getChildFile("FlowZone_Logs");
    if (!logDir.exists())
      logDir.createDirectory();

    logPath_ = logDir.getFullPathName().toStdString();

    // Open log files (truncate on each app launch)
    wsStream_.open(logPath_ + "/ws_server.log", std::ios::trunc);
    audioStream_.open(logPath_ + "/audio_flow.log", std::ios::trunc);
    stateStream_.open(logPath_ + "/state_broadcast.log", std::ios::trunc);

    if (wsStream_.is_open())
      wsStream_ << getTimestamp() << " === FlowZone WebSocket Log Started ===\n";
    if (audioStream_.is_open())
      audioStream_ << getTimestamp()
                   << " === FlowZone Audio Flow Log Started ===\n";
    if (stateStream_.is_open())
      stateStream_ << getTimestamp()
                   << " === FlowZone State Broadcast Log Started ===\n";
  }

  ~FileLogger() {
    wsStream_.close();
    audioStream_.close();
    stateStream_.close();
  }

  std::ofstream &getStream(Category cat) {
    switch (cat) {
    case Category::WebSocket:
      return wsStream_;
    case Category::AudioFlow:
      return audioStream_;
    case Category::StateBroadcast:
      return stateStream_;
    }
    return wsStream_; // fallback
  }

  std::string getTimestamp() {
    auto now = juce::Time::getCurrentTime();
    return now.formatted("%Y-%m-%d %H:%M:%S").toStdString() + "." +
           std::to_string(now.toMilliseconds() % 1000);
  }

  std::mutex mutex_;
  std::string logPath_;
  std::ofstream wsStream_;
  std::ofstream audioStream_;
  std::ofstream stateStream_;

  FileLogger(const FileLogger &) = delete;
  FileLogger &operator=(const FileLogger &) = delete;
};

} // namespace flowzone
