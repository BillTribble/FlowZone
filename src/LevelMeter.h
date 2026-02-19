#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/// A vertical level meter with smooth ballistics and peak hold.
/// Call setLevel() from the message thread (via Timer) with the current peak.
class LevelMeter : public juce::Component
{
public:
    LevelMeter();

    void setLevel (float newLevel);
    void paint (juce::Graphics& g) override;

private:
    float currentLevel = 0.0f;
    float peakHoldLevel = 0.0f;
    float peakDecayRate = 0.01f;     // per-frame decay for the display level
    float peakHoldDecayRate = 0.005f; // per-frame decay for peak hold marker

    juce::Colour getColourForLevel (float level) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
