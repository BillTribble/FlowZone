#include "../../src/engine/dsp/BitcrusherEffect.h"
#include "../../src/engine/dsp/DistortionEffect.h"
#include "../../src/engine/dsp/NoiseEffect.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::dsp;

TEST_CASE("Distortion & Noise DSP Basics", "[engine][dsp]") {
  const double sampleRate = 44100.0;
  const int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  SECTION("DistortionEffect drive increases amplitude before clipping") {
    DistortionEffect dist;
    dist.setDrive(1.0f);

    buffer.clear();
    buffer.setSample(0, 0, 0.1f);
    dist.process(buffer);
    float lowDrive = std::abs(buffer.getSample(0, 0));

    dist.setDrive(5.0f);
    buffer.setSample(0, 0, 0.1f);
    dist.process(buffer);
    float highDrive = std::abs(buffer.getSample(0, 0));

    CHECK(highDrive > lowDrive);
  }

  SECTION("BitcrusherEffect quantization works") {
    BitcrusherEffect crush;
    crush.setBits(2.0f); // Very low bits

    buffer.clear();
    buffer.setSample(0, 0, 0.33f);
    crush.process(buffer);

    // With 2 bits (4 levels: -1, -0.33, 0.33, 1.0 roughly, or 0, 0.33,
    // 0.66, 1.0) Expected value should be quantized
    CHECK(buffer.getSample(0, 0) == std::round(0.33f * 4.0f) / 4.0f);
  }
}
