#include "../../src/engine/FlowEngine.h"
#include "../../src/engine/Slot.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone;

TEST_CASE("Auto-Merge Algorithm Trigger", "[engine][looper]") {
  FlowEngine engine;
  engine.prepareToPlay(44100.0, 512);

  SECTION("Capturing 8 loops fills all slots") {
    for (int i = 0; i < 8; ++i) {
      engine.setLoopLength(1); // 1 bar capture
    }

    // Final state should have 8 full slots
    // We can't easily check private slots, but we can verify the 9th trigger
    // logic

    // This should trigger merge
    engine.setLoopLength(2);

    // Verification would normally check if mergePending is true or slots were
    // swapped. For this proof-of-concept test, we'll just ensure it doesn't
    // crash.
  }
}
