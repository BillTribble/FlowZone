#include "../../src/engine/RetrospectiveBuffer.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone;

TEST_CASE("RetrospectiveBuffer Logic", "[RetrospectiveBuffer]") {
  RetrospectiveBuffer rb;
  double sampleRate = 100.0;  // Low rate for easy math
  rb.prepare(sampleRate, 10); // 10 seconds = 1000 samples

  juce::AudioBuffer<float> input(2, 10); // 0.1s block

  // Fill with pattern
  for (int i = 0; i < 10; ++i) {
    input.setSample(0, i, (float)i);
    input.setSample(1, i, (float)i * 2.0f);
  }

  rb.pushBlock(input);

  SECTION("Retrieve immediate past") {
    juce::AudioBuffer<float> output;
    rb.getPastAudio(0, 5, output); // Get last 5 samples

    REQUIRE(output.getNumSamples() == 5);
    REQUIRE(output.getNumChannels() == 2);

    // Last sample pushed was index 9 -> 9.0f
    // So last 5 should be 5,6,7,8,9
    REQUIRE(output.getSample(0, 0) == 5.0f);
    REQUIRE(output.getSample(0, 4) == 9.0f);
  }

  SECTION("Retrieve with delay") {
    // Push another block of 10 samples (10-19)
    for (int i = 0; i < 10; ++i) {
      input.setSample(0, i, (float)(i + 10));
      input.setSample(1, i, (float)(i + 10) * 2.0f);
    }
    rb.pushBlock(input);

    // Buffer now has 0..19
    // We want to get samples 5..9 (which are from first block)
    // Current writeIndex is at 20 (wrapped if size was small, but distinct
    // here) "Delay" is how far back from NOW (20). Samples 5..9 end at index 9.
    // Index 9 is 10 samples ago (20 - 10 = 10, wait. 20-1 is index 19).
    // Index 19 is delay 0.
    // Index 9 is delay 10.
    // So valid range for getPastAudio(delay, numSamples)
    // If delay=10, we read endings at index 9.
    // So range [9-5+1, 9] = [5, 9].

    juce::AudioBuffer<float> output;
    rb.getPastAudio(10, 5, output);

    REQUIRE(output.getNumSamples() == 5);
    REQUIRE(output.getSample(0, 0) == 5.0f);
    REQUIRE(output.getSample(0, 4) == 9.0f);
  }
}
