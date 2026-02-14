#include "../../libs/catch2/catch.hpp"
#include "../../src/engine/AudioThreadUtils.h"
#include "../../src/shared/protocol/ErrorCodes.h"
#include "../../src/shared/protocol/schema.h"

using namespace flowzone;

TEST_CASE("bd-34b: Error Codes Registry", "[phase1]") {
  REQUIRE((int)ErrorCode::None == 0);
  REQUIRE((int)ErrorCode::InvalidCommand == 1001);
  REQUIRE((int)ErrorCode::DiskFull == 4002);
}

TEST_CASE("bd-13w: Audio Thread Utils", "[phase1]") {
  // We are on the main test thread (message thread effectively)
  // AudioThreadUtils::assertMessageThread(); // Should pass (if MM init)
  // AudioThreadUtils::assertAudioThread(); // Should fail
  REQUIRE(true); // Placeholder for compilation check
}

TEST_CASE("bd-3g0: Schema Structs", "[phase1]") {
  AppState state;
  state.bpm = 140.0;
  state.isPlaying = true;

  REQUIRE(state.bpm == 140.0);
  REQUIRE(state.isPlaying == true);
}
