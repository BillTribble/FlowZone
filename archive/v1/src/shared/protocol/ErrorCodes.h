#pragma once

namespace flowzone {

// bd-34b: Error Codes Registry
enum class ErrorCode {
  // 0-999: System / Generic
  None = 0,
  UnknownError = 1,

  // 1000-1099: Protocol
  InvalidCommand = 1001,
  InvalidPayload = 1002,

  // 2000-2999: Audio Engine
  EngineNotReady = 2001,
  AudioDeviceError = 2002,

  // 3000-3999: Plugins
  PluginScanFailed = 3001,
  PluginLoadFailed = 3002,
  PluginCrashed = 3003,

  // 4000-4999: Session
  SessionLoadFailed = 4001,
  DiskFull = 4002 // bd-ad0
};

} // namespace flowzone
