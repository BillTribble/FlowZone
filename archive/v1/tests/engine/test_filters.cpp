#include "../../src/engine/dsp/CombFilter.h"
#include "../../src/engine/dsp/MulticombFilter.h"
#include "../../src/engine/dsp/StateVariableFilter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace flowzone::dsp;

TEST_CASE("Filter DSP Basics", "[engine][dsp]") {
  const double sampleRate = 44100.0;
  const int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  SECTION("StateVariableFilter (Lowpass) suppresses high frequencies") {
    StateVariableFilter svf(StateVariableFilter::Lowpass);
    svf.prepare(sampleRate, blockSize);
    svf.setCutoff(100.0f); // Very low cutoff

    // Fill with white noise
    juce::Random rng;
    for (int ch = 0; ch < 2; ++ch) {
      for (int i = 0; i < blockSize; ++i)
        buffer.setSample(ch, i, rng.nextFloat() * 2.0f - 1.0f);
    }

    float initialRms = buffer.getRMSLevel(0, 0, blockSize);
    svf.process(buffer);
    float filteredRms = buffer.getRMSLevel(0, 0, blockSize);

    CHECK(filteredRms < initialRms);
  }

  SECTION("CombFilter feedback produces decay") {
    CombFilter comb;
    comb.prepare(sampleRate, blockSize);
    comb.setDelay(10.0f);
    comb.setFeedback(0.9f);

    buffer.clear();
    buffer.setSample(0, 0, 1.0f); // Impulse

    comb.process(buffer);

    // After first block, we should have some signal in the buffer if delay <
    // block length
    CHECK(buffer.getRMSLevel(0, 0, blockSize) > 0.0f);
  }
}
