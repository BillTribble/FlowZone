#pragma once

#include "ErrorCodes.h"
#include <string>
#include <vector>

namespace flowzone {

// Command Types
enum class CommandType { Play, Pause, SetBpm, LoadRiff, Unknown };

// Data Structures
struct Riff {
  std::string id;
  std::string name;
  double lengthBeats;
  // ... more fields
};

struct AppState {
  double bpm = 120.0;
  bool isPlaying = false;
  std::vector<Riff> activeRiffs;
};

} // namespace flowzone
