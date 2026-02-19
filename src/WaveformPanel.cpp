#include "WaveformPanel.h"
#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
WaveformPanel::WaveformPanel() { setOpaque(true); }

//==============================================================================
void WaveformPanel::setSectionData(int sectionIndex,
                                   const std::vector<float> &data) {
  if (sectionIndex >= 0 && sectionIndex < 4) {
    sectionData[static_cast<size_t>(sectionIndex)] = data;
    repaint();
  }
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

  if (w < 40.0f)
    return;

  const float sectionW = w / 4.0f;
  const float midY = h * 0.5f;
  const float scaleY = (h * 0.45f);

  // Labels for the 4 sections: 8, 4, 2, 1 bars
  constexpr std::array<int, 4> sectionBars{8, 4, 2, 1};

  for (int i = 0; i < 4; ++i) {
    const float sectionX = i * sectionW;
    const auto sectionRect = juce::Rectangle<float>(sectionX, 0, sectionW, h);

    // --- Vertical Divider (except for the first one) ---
    if (i > 0) {
      g.setColour(juce::Colours::white.withAlpha(0.12f));
      for (float y = 0; y < h; y += 8.0f)
        g.drawLine(sectionX, y, sectionX, std::min(y + 4.0f, h), 1.0f);
    }

    // --- Label ---
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(juce::String(sectionBars[i]), sectionX, 4, sectionW, 14,
               juce::Justification::centred, false);

    // --- Waveform ---
    const auto &data = sectionData[static_cast<size_t>(i)];
    if (data.empty())
      continue;

    const int numPoints = static_cast<int>(data.size());
    juce::Path wavePath;
    bool first = true;

    for (int pt = 0; pt < numPoints; ++pt) {
      float x = sectionX + (static_cast<float>(pt) /
                            static_cast<float>(std::max(1, numPoints - 1))) *
                               sectionW;
      float peak = std::clamp(data[static_cast<size_t>(pt)], 0.0f, 1.0f);
      float yTop = midY - peak * scaleY;

      if (first) {
        wavePath.startNewSubPath(x, midY);
        first = false;
      }
      wavePath.lineTo(x, yTop);
    }

    // Mirror path back along the bottom
    for (int pt = numPoints - 1; pt >= 0; --pt) {
      float x = sectionX + (static_cast<float>(pt) /
                            static_cast<float>(std::max(1, numPoints - 1))) *
                               sectionW;
      float peak = std::clamp(data[static_cast<size_t>(pt)], 0.0f, 1.0f);
      float yBottom = midY + peak * scaleY;
      wavePath.lineTo(x, yBottom);
    }
    wavePath.closeSubPath();

    // Fill with section-specific gradient alpha or color if needed (using green
    // for now)
    juce::ColourGradient waveGrad(juce::Colour(0xFF00CC66).withAlpha(0.75f),
                                  sectionX, midY - scaleY,
                                  juce::Colour(0xFF007744).withAlpha(0.45f),
                                  sectionX, midY + scaleY, false);
    g.setGradientFill(waveGrad);
    g.fillPath(wavePath);

    // Outline
    juce::Path outlinePath;
    first = true;
    for (int pt = 0; pt < numPoints; ++pt) {
      float x = sectionX + (static_cast<float>(pt) /
                            static_cast<float>(std::max(1, numPoints - 1))) *
                               sectionW;
      float peak = std::clamp(data[static_cast<size_t>(pt)], 0.0f, 1.0f);
      float yTop = midY - peak * scaleY;
      if (first) {
        outlinePath.startNewSubPath(x, yTop);
        first = false;
      } else
        outlinePath.lineTo(x, yTop);
    }
    g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.6f));
    g.strokePath(outlinePath, juce::PathStrokeType(1.1f));
  }

  // Centre baseline
  g.setColour(juce::Colours::white.withAlpha(0.06f));
  g.drawLine(0.0f, midY, w, midY, 1.0f);
}
