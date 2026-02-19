#include "../../src/engine/DrumEngine.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::engine;

TEST_CASE("Drum Engine Basics", "[engine][drums]") {
  DrumEngine engine;
  const double sampleRate = 44100.0;
  const int blockSize = 512;

  engine.prepare(sampleRate, blockSize);

  SECTION("Renders silence when no midi") {
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    juce::MidiBuffer midi;

    engine.process(buffer, midi);

    for (int ch = 0; ch < 2; ++ch) {
      for (int i = 0; i < blockSize; ++i) {
        CHECK(buffer.getSample(ch, i) == 0.0f);
      }
    }
  }

  SECTION("Renders audio on Note On (Pad 0 - Kick)") {
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);

    engine.process(buffer, midi);

    bool foundSignal = false;
    for (int i = 0; i < blockSize; ++i) {
      if (std::abs(buffer.getSample(0, i)) > 0.01f) {
        foundSignal = true;
        break;
      }
    }
    CHECK(foundSignal);
  }
}
