#include "RiffHistoryPanel.h"
#include <algorithm>

RiffHistoryPanel::RiffHistoryPanel() { setOpaque(true); }

void RiffHistoryPanel::setHistory(const RiffHistory *history) {
  riffHistory = history;
  updateItems();
  repaint();
}

void RiffHistoryPanel::updateItems() {
  items.clear();
  if (riffHistory == nullptr)
    return;

  const auto &history = riffHistory->getHistory();
  const float w = static_cast<float>(getWidth());
  const float h = static_cast<float>(getHeight());

  // Fixed width for each riff box
  const float itemW = 80.0f;
  const float spacing = 10.0f;

  // Render from right to left
  float currentX = w - itemW - 10.0f;

  // History is oldest to newest, so we iterate backwards to put newest on the
  // right
  for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i) {
    if (currentX < -itemW)
      break; // Off screen

    const auto &riff = history[static_cast<size_t>(i)];
    RiffItem item;
    item.riff = &riff;
    item.bounds = juce::Rectangle<float>(currentX, 10.0f, itemW, h - 20.0f);
    item.thumbnail = generateThumbnail(riff.audio, static_cast<int>(itemW));

    items.push_back(std::move(item));
    currentX -= (itemW + spacing);
  }
}

std::vector<float>
RiffHistoryPanel::generateThumbnail(const juce::AudioBuffer<float> &audio,
                                    int numPoints) {
  std::vector<float> result(static_cast<size_t>(numPoints), 0.0f);
  const int numSamples = audio.getNumSamples();
  if (numSamples <= 0 || numPoints <= 0)
    return result;

  const double samplesPerPoint =
      static_cast<double>(numSamples) / static_cast<double>(numPoints);
  const int numChannels = audio.getNumChannels();

  for (int i = 0; i < numPoints; ++i) {
    int start = static_cast<int>(i * samplesPerPoint);
    int end = static_cast<int>((i + 1) * samplesPerPoint);
    end = std::min(end, numSamples);

    float peak = 0.0f;
    for (int s = start; s < end; ++s) {
      for (int ch = 0; ch < numChannels; ++ch) {
        float absS = std::abs(audio.getSample(ch, s));
        if (absS > peak)
          peak = absS;
      }
    }
    result[static_cast<size_t>(i)] = peak;
  }
  return result;
}

void RiffHistoryPanel::paint(juce::Graphics &g) {
  // Update items here if history size changed (naive but works for now)
  if (riffHistory && items.size() != riffHistory->size()) {
    updateItems();
  }

  const auto bounds = getLocalBounds().toFloat();
  const float h = bounds.getHeight();

  // Background
  g.setColour(juce::Colour(0xFF0A0A1A));
  g.fillRect(bounds);

  // Separator
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  g.drawLine(0, 0, bounds.getWidth(), 0, 1.0f);

  for (size_t i = 0; i < items.size(); ++i) {
    const auto &item = items[i];
    const bool isSelected = (static_cast<int>(i) == selectedRiffIndex);

    // Box background
    g.setColour(juce::Colour(0xFF16162B));
    g.fillRoundedRectangle(item.bounds, 6.0f);

    if (isSelected) {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRoundedRectangle(item.bounds, 6.0f);
      g.setColour(juce::Colour(0xFF00CC66));
      g.drawRoundedRectangle(item.bounds, 6.0f, 2.0f);
    } else {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.drawRoundedRectangle(item.bounds, 6.0f, 1.0f);
    }

    // Mini-waveform
    const float midY = item.bounds.getCentreY();
    const float scaleY = item.bounds.getHeight() * 0.35f;

    g.setColour(juce::Colour(0xFF00CC66).withAlpha(0.6f));
    for (int pt = 0; pt < static_cast<int>(item.thumbnail.size()); ++pt) {
      float x = item.bounds.getX() + static_cast<float>(pt);
      float peak =
          std::clamp(item.thumbnail[static_cast<size_t>(pt)], 0.0f, 1.0f);
      g.drawLine(x, midY - peak * scaleY, x, midY + peak * scaleY, 1.0f);
    }

    // Name
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(item.riff->name, item.bounds.reduced(4, 2),
               juce::Justification::bottomLeft, false);
  }

  if (items.empty() && riffHistory) {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::italic));
    g.drawText("No riffs yet...", bounds, juce::Justification::centred, false);
  }
}

void RiffHistoryPanel::resized() { updateItems(); }

void RiffHistoryPanel::mouseDown(const juce::MouseEvent &e) {
  for (int i = 0; i < static_cast<int>(items.size()); ++i) {
    if (items[static_cast<size_t>(i)].bounds.contains(e.position)) {
      selectedRiffIndex = i;
      if (onRiffSelected)
        onRiffSelected(*items[static_cast<size_t>(i)].riff);
      repaint();
      return;
    }
  }
}
