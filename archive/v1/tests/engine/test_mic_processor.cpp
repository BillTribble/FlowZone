#include "../../src/engine/MicProcessor.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::engine;

TEST_CASE("Mic Processor Basics", "[engine][mic]") {
  MicProcessor mic;
  const double sampleRate = 44100.0;
  const int blockSize = 512;

  mic.prepare(sampleRate, blockSize);

  SECTION("Input Gain works") {
    juce::AudioBuffer<float> input(1, blockSize);
    juce::AudioBuffer<float> output(2, blockSize);
    input.clear();
    output.clear();

    // Fill input with 0.1
    for (int i = 0; i < blockSize; ++i)
      input.setSample(0, i, 0.1f);

    // Set gain +6dB (~2x)
    mic.setInputGain(6.0f);
    mic.setMonitorEnabled(true);
    mic.process(input, output);

    CHECK(output.getSample(0, 0) == Approx(0.2f).margin(0.01));
  }

  SECTION("Direct Monitoring Toggle") {
    juce::AudioBuffer<float> input(1, blockSize);
    juce::AudioBuffer<float> output(2, blockSize);
    for (int i = 0; i < blockSize; ++i)
      input.setSample(0, i, 0.1f);
    output.clear();

    mic.setMonitorEnabled(false);
    mic.process(input, output);
    CHECK(output.getSample(0, 0) == 0.0f);

    mic.setMonitorEnabled(true);
    mic.process(input, output);
    CHECK(output.getSample(0, 0) == Approx(0.1f));
  }
}
