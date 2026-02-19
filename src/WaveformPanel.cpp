#include "WaveformPanel.h"
#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
WaveformPanel::WaveformPanel() { setOpaque(true); }

//==============================================================================
void WaveformPanel::setWaveformData(const std::vector<float> &data) {
  waveformData = data;
  repaint();
}

void WaveformPanel::setBPM(double bpm) {
  currentBPM = (bpm > 1.0) ? bpm : 120.0;
  repaint();
}

void WaveformPanel::setSampleRate(double sampleRate) {
  currentSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
  repaint();
}

//==============================================================================
void WaveformPanel::resized() { repaint(); }

//==============================================================================
void WaveformPanel::paint(juce::Graphics &g) {
  const auto bounds = getLocalBounds().toFloat();
  const float w = bounds.getWidth();
  const float h = bounds.getHeight();

  // --- Background: dark gradient ---
  juce::ColourGradient bg(juce::Colour(0xFF0F0F23), 0.0f, 0.0f,
                          juce::Colour(0xFF080814), 0.0f, h, false);
  g.setGradientFill(bg);
  g.fillRect(bounds);

  // Thin top border
  g.setColour(juce::Colours::white.withAlpha(0.08f));
  g.drawLine(0.0f, 0.0f, w, 0.0f, 1.0f);

  // --- Bar division lines ---
  // Bar width in samples at current BPM (4/4 signature assumed)
  const double framesPerBeat = currentSampleRate * (60.0 / currentBPM);
  const double framesPerBar = framesPerBeat * 4.0;

  // Total window = 8 bars worth of samples
  const double totalFrames = framesPerBar * 8.0;

  // The waveformData vector spans totalFrames of audio mapped to [0, w] pixels.
  // Division at 1, 2, 4, 8 bars from right edge.
  constexpr std::array<int, 4> divBars{1, 2, 4, 8};

  for (int bars : divBars) {
    double fractionFromRight = (framesPerBar * bars) / totalFrames;
    float xPos = w - static_cast<float>(fractionFromRight * w);
    if (xPos < 0.0f || xPos > w)
      continue;

    // Dashed vertical line
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    for (float y = 4.0f; y < h; y += 8.0f)
      g.drawLine(xPos, y, xPos, std::min(y + 5.0f, h), 1.0f);

    // Label above
    juce::String label = juce::String(bars);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(label, static_cast<int>(xPos) - 8, 3, 16, 14,
               juce::Justification::centred, false);
  }

  // --- Waveform path ---
  if (waveformData.empty() || w < 2.0f)
    return;

  const int numPoints = static_cast<int>(waveformData.size());
  const float midY = h * 0.5f;
  const float scaleY = (h * 0.45f); // leave a little padding top/bottom

  // Use a filled path for a nicer look (mirrored around centre line)
  juce::Path wavePath;
  bool first = true;

  for (int i = 0; i < numPoints; ++i) {
    float x = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
    float peak = std::clamp(waveformData[static_cast<size_t>(i)], 0.0f, 1.0f);
    float yTop = midY - peak * scaleY;

    if (first) {
      wavePath.startNewSubPath(x, midY);
      first = false;
    }
    wavePath.lineTo(x, yTop);
  }

  // Trace back along the bottom (mirror)
  for (int i = numPoints - 1; i >= 0; --i) {
    float x = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
    float peak = std::clamp(waveformData[static_cast<size_t>(i)], 0.0f, 1.0f);
    float yBottom = midY + peak * scaleY;
    wavePath.lineTo(x, yBottom);
  }
  wavePath.closeSubPath();

  // Green gradient fill
  juce::ColourGradient waveGrad(
      juce::Colour(0xFF00CC66).withAlpha(0.85f), 0.0f, midY - scaleY,
      juce::Colour(0xFF007744).withAlpha(0.55f), 0.0f, midY + scaleY, false);
  g.setGradientFill(waveGrad);
  g.fillPath(wavePath);

  // Bright outline on top half only
  juce::Path outlinePath;
  first = true;
  for (int i = 0; i < numPoints; ++i) {
    float x = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
    float peak = std::clamp(waveformData[static_cast<size_t>(i)], 0.0f, 1.0f);
    float yTop = midY - peak * scaleY;
    if (first) {
      outlinePath.startNewSubPath(x, yTop);
      first = false;
    } else
      outlinePath.lineTo(x, yTop);
  }
  g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.7f));
  g.strokePath(outlinePath,
               juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Centre baseline
  g.setColour(juce::Colours::white.withAlpha(0.06f));
  g.drawLine(0.0f, midY, w, midY, 1.0f);
}
