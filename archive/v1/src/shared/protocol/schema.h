#pragma once

#include "ErrorCodes.h"
#include <string>
#include <vector>

namespace flowzone {

// ErrorCode included from ErrorCodes.h

enum class CommandType { Play, Pause, SetBpm, LoadRiff, Unknown };

// Data definitions moved to AppState.h to avoid conflicts/redeclarations

// ... (We could add full C++ structs here if needed by CommandDispatcher,
// but for now, let's just ensure the enums used by CommandDispatcher are
// present)

} // namespace flowzone
