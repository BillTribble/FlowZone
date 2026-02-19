#include "../../src/engine/dsp/ReverseEffect.h"
#include "../../src/engine/dsp/StutterEffect.h"
#include "../../src/engine/dsp/TapeStopEffect.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::dsp;

TEST_CASE("Performance DSP Basics", "[engine][dsp]") {
  const double sampleRate = 44100.0;
  const int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  SECTION("StutterEffect repeats same block") {
    StutterEffect stutter;
    stutter.prepare(sampleRate, blockSize);
    stutter.setStutterSize(blockSize * 1000.0 /
                           sampleRate); // Exactly one block size

    // Fill first block
    for (int i = 0; i < blockSize; ++i)
      buffer.setSample(0, i, (float)i / blockSize);
    stutter.process(buffer);

    // Enable stutter
    stutter.setEnabled(true);
    juce::AudioBuffer<float> buffer2(2, blockSize);
    buffer2.clear();
    stutter.process(buffer2);

    // buffer2 should match original buffer
    CHECK(buffer2.getSample(0, 100) == buffer.getSample(0, 100));
  }

  SECTION("ReverseEffect plays backwards") {
    ReverseEffect rev;
    rev.prepare(sampleRate, blockSize);

    // Fill history
    for (int i = 0; i < blockSize; ++i)
      buffer.setSample(0, i, (float)i / blockSize);
    rev.process(buffer);

    // Enable reverse
    rev.setEnabled(true);
    buffer.clear();
    rev.process(buffer);

    // First sample of buffer should be last sample of history
    CHECK(buffer.getSample(0, 0) ==
          Approx((float)(blockSize - 1) / blockSize).margin(0.01));
  }
}
