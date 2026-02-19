#include "../../src/engine/dsp/DelayEffect.h"
#include "../../src/engine/dsp/DubDelay.h"
#include "../../src/engine/dsp/ZapDelay.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::dsp;

TEST_CASE("Delay DSP Basics", "[engine][dsp]") {
  const double sampleRate = 44100.0;
  const int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  SECTION("DelayEffect produces output after initial silence") {
    DelayEffect delay;
    delay.prepare(sampleRate, blockSize);
    delay.setDelayMs(10.0f); // ~441 samples
    delay.setFeedback(0.5f);
    delay.setMix(1.0f); // Wet only

    buffer.clear();
    buffer.setSample(0, 0, 1.0f); // Impulse at start

    delay.process(buffer);

    // Since delay is 10ms (441 samples), and block is 512, the impulse should
    // appear near the end.
    bool foundSignal = false;
    for (int i = 0; i < blockSize; ++i) {
      if (std::abs(buffer.getSample(0, i)) > 0.01f) {
        foundSignal = true;
        break;
      }
    }
    CHECK(foundSignal);
  }

  SECTION("DubDelay saturation clips peaks") {
    DubDelay dub;
    dub.prepare(sampleRate, blockSize);
    dub.setSaturation(1.0f);
    dub.setMix(1.0f);
    dub.setDelayMs(0.01f); // Micro delay
    dub.setFeedback(0.0f);

    buffer.clear();
    for (int i = 0; i < blockSize; ++i)
      buffer.setSample(0, i, 1.5f); // Hot signal

    dub.process(buffer);

    // Max output of std::tanh is 1.0
    for (int i = 0; i < blockSize; ++i) {
      CHECK(std::abs(buffer.getSample(0, i)) <= 1.0f);
    }
  }
}
