#include "LevelMeter.h"

LevelMeter::LevelMeter() {}

void LevelMeter::setLevel(float newLevel) {
  // Smooth attack (instant) and decay (gradual)
  if (newLevel > currentLevel)
    currentLevel = newLevel;
  else
    currentLevel = currentLevel * 0.85f + newLevel * 0.15f;

  // Peak hold: only update if new peak is higher
  if (newLevel > peakHoldLevel)
    peakHoldLevel = newLevel;
  else
    peakHoldLevel = std::max(0.0f, peakHoldLevel - peakHoldDecayRate);

  repaint();
}

juce::Colour LevelMeter::getColourForLevel(float level) const {
  if (level > 0.9f)
    return juce::Colours::red;
  if (level > 0.6f)
    return juce::Colour(0xFFFFAA00); // amber/yellow
  return juce::Colour(0xFF00CC66);   // green
}

void LevelMeter::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(2.0f);

  // Background
  g.setColour(juce::Colour(0xFF1A1A2E));
  g.fillRoundedRectangle(bounds, 6.0f);

  // Dark inset border
  g.setColour(juce::Colour(0xFF0D0D1A));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  auto meterArea = bounds.reduced(4.0f);
  float level = juce::jlimit(0.0f, 1.0f, currentLevel);

  if (isHorizontal) {
    float meterWidth = meterArea.getWidth();
    float filledWidth = meterWidth * level;

    if (filledWidth > 0.0f) {
      auto filledBounds = meterArea.removeFromLeft(filledWidth);
      juce::ColourGradient gradient(
          juce::Colour(0xFF00CC66), filledBounds.getTopLeft(),
          juce::Colours::red, filledBounds.getTopRight(), false);
      gradient.addColour(0.6, juce::Colour(0xFFFFAA00));

      g.setGradientFill(gradient);
      g.fillRoundedRectangle(filledBounds, 4.0f);
    }

    // Peak hold line
    if (peakHoldLevel > 0.01f) {
      float peakX = meterArea.getX() +
                    (meterWidth * juce::jlimit(0.0f, 1.0f, peakHoldLevel));
      g.setColour(juce::Colours::white.withAlpha(0.8f));
      g.fillRect(peakX - 1.0f, meterArea.getY(), 2.0f, meterArea.getHeight());
    }
  } else {
    // Original Vertical Logic
    float meterHeight = meterArea.getHeight();
    float filledHeight = meterHeight * level;

    if (filledHeight > 0.0f) {
      auto filledBounds = meterArea.removeFromBottom(filledHeight);
      juce::ColourGradient gradient(
          juce::Colour(0xFF00CC66), filledBounds.getBottomLeft(),
          juce::Colours::red, filledBounds.getTopLeft(), false);
      gradient.addColour(0.6, juce::Colour(0xFFFFAA00));

      g.setGradientFill(gradient);
      g.fillRoundedRectangle(filledBounds, 4.0f);
    }

    // Peak hold indicator line
    if (peakHoldLevel > 0.01f) {
      float peakY = bounds.getBottom() - 4.0f -
                    (meterHeight * juce::jlimit(0.0f, 1.0f, peakHoldLevel));
      g.setColour(juce::Colours::white.withAlpha(0.8f));
      g.fillRect(bounds.getX() + 4.0f, peakY, bounds.getWidth() - 8.0f, 2.0f);
    }

    // Original dB markings for vertical (only if enough height)
    if (bounds.getHeight() > 40.0f) {
      g.setColour(juce::Colours::white.withAlpha(0.3f));
      g.setFont(10.0f);
      g.drawText("0", (int)(bounds.getX() + 2), (int)(bounds.getY() + 4.0f),
                 (int)bounds.getWidth() - 4, 12, juce::Justification::centred,
                 false);
      float neg6Y = bounds.getBottom() - 4.0f - meterHeight * 0.5f;
      g.drawText("-6", (int)(bounds.getX() + 2), (int)neg6Y,
                 (int)bounds.getWidth() - 4, 12, juce::Justification::centred,
                 false);
    }
  }
}
