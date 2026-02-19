#include "../../src/engine/transport/TransportService.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("TransportService State Management", "[TransportService]") {
  TransportService transport;
  transport.prepareToPlay(44100.0, 512);

  SECTION("Initial State") {
    REQUIRE(transport.getBpm() == 120.0);
    REQUIRE(transport.getLoopLengthBars() == 4);
    REQUIRE_FALSE(transport.isPlaying());
    REQUIRE_FALSE(transport.isMetronomeEnabled());
    REQUIRE_FALSE(transport.isQuantiseEnabled());
  }

  SECTION("BPM Setter") {
    transport.setBpm(140.0);
    REQUIRE(transport.getBpm() == 140.0);
  }

  SECTION("Loop Length Setter") {
    transport.setLoopLengthBars(8);
    REQUIRE(transport.getLoopLengthBars() == 8);

    transport.setLoopLengthBars(0); // Should clamp to 1
    REQUIRE(transport.getLoopLengthBars() == 1);
  }

  SECTION("Playback Control") {
    transport.play();
    REQUIRE(transport.isPlaying());

    transport.pause();
    REQUIRE_FALSE(transport.isPlaying());

    transport.togglePlay();
    REQUIRE(transport.isPlaying());
  }
}

TEST_CASE("TransportService Processing", "[TransportService]") {
  TransportService transport;
  transport.prepareToPlay(44100.0, 512);
  transport.play();

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;

  SECTION("Advances Time") {
    double initialPpq = transport.getPpqPosition();
    transport.processBlock(buffer, midi);
    REQUIRE(transport.getPpqPosition() > initialPpq);
  }

  SECTION("Metronome Audio Generation") {
    transport.setMetronomeEnabled(true);
    buffer.clear();

    // Process enough blocks to ensure a click happens
    // At 120bpm, 1 beat = 0.5s = 22050 samples.
    // We need to process ~44 blocks of 512 samples.

    bool signalFound = false;

    for (int i = 0; i < 100; ++i) {
      transport.processBlock(buffer, midi);
      if (buffer.getMagnitude(0, 512) > 0.0f) {
        signalFound = true;
        break;
      }
    }

    REQUIRE(signalFound);
  }
}
