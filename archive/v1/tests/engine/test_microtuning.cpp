#include "../../src/engine/TuningManager.h"
#include <catch2/catch_test_macros.hpp>

using namespace flowzone::engine;

TEST_CASE("Microtuning Basics", "[engine][tuning]") {
  TuningManager tm;

  SECTION("12TET default frequencies") {
    tm.setTo12TET();
    // A4 (69) should be 440Hz
    CHECK(tm.getFrequencyForMidiNote(69) == Approx(440.0));
    // A5 (81) should be 880Hz
    CHECK(tm.getFrequencyForMidiNote(81) == Approx(880.0));
  }

  SECTION("Just Intonation frequencies") {
    tm.setToJustIntonation();
    // Major third (C4 to E4 -> 60 to 64) ratio should be 5/4
    double c4 = tm.getFrequencyForMidiNote(60);
    double e4 = tm.getFrequencyForMidiNote(64);
    CHECK(e4 / c4 == Approx(1.25));
  }

  SECTION("Parse SCL content (Pythagorean 7-note)") {
    juce::String scl = "! pyth7.scl\n"
                       "Pythagorean 7-note\n"
                       "7\n"
                       "9/8\n"
                       "81/64\n"
                       "4/3\n"
                       "3/2\n"
                       "27/16\n"
                       "243/128\n"
                       "2/1\n";

    tm.loadScl(scl);
    // Note 69 is root, 70 is 9/8
    double f69 = tm.getFrequencyForMidiNote(69);
    double f70 = tm.getFrequencyForMidiNote(70);
    CHECK(f70 / f69 == Approx(1.125));
  }
}
