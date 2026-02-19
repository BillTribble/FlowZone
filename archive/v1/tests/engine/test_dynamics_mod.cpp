#include "../../src/engine/dsp/ChorusEffect.h"
#include "../../src/engine/dsp/CompressorEffect.h"
#include "../../src/engine/dsp/LimiterEffect.h"
#include "../../src/engine/dsp/PhaserEffect.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::dsp;

TEST_CASE("Dynamics & Modulation DSP Basics", "[engine][dsp]") {
  const double sampleRate = 44100.0;
  const int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  SECTION("Compressor reduces gain of hot signals") {
    CompressorEffect comp;
    comp.prepare(sampleRate, blockSize);
    comp.setThreshold(-20.0f);
    comp.setRatio(10.0f);
    comp.setAttack(1.0f);

    // Hot signal (0dB)
    for (int ch = 0; ch < 2; ++ch) {
      for (int i = 0; i < blockSize; ++i)
        buffer.setSample(ch, i, 1.0f);
    }

    comp.process(buffer);

    // Output should be less than 1.0
    CHECK(buffer.getSample(0, blockSize - 1) < 1.0f);
  }

  SECTION("Chorus effect creates stereo difference") {
    ChorusEffect chorus;
    chorus.prepare(sampleRate, blockSize);
    chorus.setMix(1.0f);
    chorus.setRate(2.0f);

    // Mono input
    for (int i = 0; i < blockSize; ++i) {
      buffer.setSample(0, i, std::sin(i * 0.1f));
      buffer.setSample(1, i, std::sin(i * 0.1f));
    }

    chorus.process(buffer);

    // Left and Right should differ
    CHECK(buffer.getSample(0, blockSize / 2) !=
          buffer.getSample(1, blockSize / 2));
  }
}
