#pragma once

#include <JuceHeader.h>
#include <vector>

namespace flowzone {
namespace engine {

/**
 * @brief Manages Microtuning / Scala (.scl) and Keyboard Mapping (.kbm).
 */
class TuningManager {
public:
  TuningManager();

  void loadScl(const juce::String &content);
  void loadKbm(const juce::String &content);

  double getFrequencyForMidiNote(int midiNoteNumber, double rootFreq = 440.0,
                                 int rootMidiNote = 69) const;

  void setTo12TET();
  void setToJustIntonation();
  void setToPythagorean();

private:
  std::vector<double> ratios; // Ratios relative to root
  int notesInOctave = 12;
  double octaveRatio = 2.0;

  // KBM basics
  int kbmMiddleNote = 69;
  int kbmReferenceNote = 69;
  double kbmReferenceFreq = 440.0;
  int kbmOctaveDegree = 0;

  void parseScl(const juce::String &content);
  double parseSclLine(const juce::String &line);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningManager)
};

} // namespace engine
} // namespace flowzone
