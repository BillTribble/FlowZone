#include "TuningManager.h"
#include <cmath>

namespace flowzone {
namespace engine {

TuningManager::TuningManager() { setTo12TET(); }

void TuningManager::loadScl(const juce::String &content) { parseScl(content); }

void TuningManager::loadKbm(const juce::String &content) {
  juce::ignoreUnused(content);
  // Placeholder for .kbm parsing
}

double TuningManager::getFrequencyForMidiNote(int midiNoteNumber,
                                              double rootFreq,
                                              int rootMidiNote) const {
  if (ratios.empty())
    return rootFreq * std::pow(2.0, (midiNoteNumber - rootMidiNote) / 12.0);

  int diff = midiNoteNumber - kbmReferenceNote;
  int octave = (int)std::floor((double)diff / notesInOctave);
  int degree = diff % notesInOctave;
  if (degree < 0)
    degree += notesInOctave;

  double ratio = (degree == 0) ? 1.0 : ratios[degree - 1];
  return kbmReferenceFreq * std::pow(octaveRatio, octave) * ratio;
}

void TuningManager::setTo12TET() {
  ratios.clear();
  for (int i = 1; i <= 11; ++i) {
    ratios.push_back(std::pow(2.0, i / 12.0));
  }
  notesInOctave = 12;
  octaveRatio = 2.0;
}

void TuningManager::setToJustIntonation() {
  ratios = {16.0 / 15.0, 9.0 / 8.0,   6.0 / 5.0, 5.0 / 4.0,
            4.0 / 3.0,   45.0 / 32.0, 3.0 / 2.0, 8.0 / 5.0,
            5.0 / 3.0,   9.0 / 5.0,   15.0 / 8.0};
  notesInOctave = 12;
  octaveRatio = 2.0;
}

void TuningManager::setToPythagorean() {
  // Basic 12-note Pythagorean
  ratios = {256.0 / 243.0, 9.0 / 8.0,     32.0 / 27.0,  81.0 / 64.0,
            4.0 / 3.0,     729.0 / 512.0, 3.0 / 2.0,    128.0 / 81.0,
            27.0 / 16.0,   16.0 / 9.0,    243.0 / 128.0};
  notesInOctave = 12;
  octaveRatio = 2.0;
}

void TuningManager::parseScl(const juce::String &content) {
  auto lines = juce::StringArray::fromLines(content);
  ratios.clear();
  int state = 0; // 0: meta, 1: count, 2: ratios

  for (auto line : lines) {
    line = line.trim();
    if (line.isEmpty() || line.startsWith("!"))
      continue;

    if (state == 0) { // Description
      state = 1;
    } else if (state == 1) { // Count
      notesInOctave = line.getIntValue();
      state = 2;
    } else if (state == 2) {
      ratios.push_back(parseSclLine(line));
    }
  }

  if (!ratios.empty()) {
    octaveRatio = ratios.back();
    ratios.pop_back(); // The last one is the octave ratio
    notesInOctave = (int)ratios.size() + 1;
  }
}

double TuningManager::parseSclLine(const juce::String &line) {
  if (line.contains(".")) {
    // Cents: ratio = 2^(cents/1200)
    double cents = line.getDoubleValue();
    return std::pow(2.0, cents / 1200.0);
  } else if (line.contains("/")) {
    // Ratio: num/den
    auto parts = juce::StringArray::fromTokens(line, "/", "");
    if (parts.size() == 2) {
      return parts[0].getDoubleValue() / parts[1].getDoubleValue();
    }
  }
  return line.getDoubleValue(); // Literal ratio
}

} // namespace engine
} // namespace flowzone
