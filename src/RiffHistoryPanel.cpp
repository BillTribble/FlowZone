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
  const auto bounds = getLocalBounds();

  if (bounds.isEmpty())
    return;

  // Update items here if history size or state changed
  if (riffHistory) {
    const int currentCount = static_cast<int>(riffHistory->getHistory().size());
    const int currentUpdate = riffHistory->getUpdateCounter();

    if (currentUpdate != lastUpdateCounter || currentCount != lastRiffCount) {
      lastUpdateCounter = currentUpdate;
      lastRiffCount = currentCount;
      updateItems();
    }
  }

  const auto boundsF = bounds.toFloat();

  // Background
  g.setColour(juce::Colour(0xFF0A0A1A));
  g.fillRect(boundsF);

  // Separator
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  g.drawLine(0.0f, 0.0f, boundsF.getWidth(), 0.0f, 1.0f);

  for (const auto &item : items) {
    const bool isSelected = (item.riff->id == selectedRiffId);
    const bool isPlaying = isRiffPlaying ? isRiffPlaying(item.riff->id) : false;

    // Draw background for riff box
    g.setColour(juce::Colour(0xFF16162B));
    g.fillRoundedRectangle(item.bounds, 6.0f);

    if (isPlaying) {
      g.setColour(juce::Colours::cyan.withAlpha(0.15f));
      g.fillRoundedRectangle(item.bounds, 6.0f);
      g.setColour(juce::Colours::cyan);
      g.drawRoundedRectangle(item.bounds, 6.0f, 2.0f);
    } else if (isSelected) {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRoundedRectangle(item.bounds, 6.0f);
      g.setColour(juce::Colour(0xFF00CC66));
      g.drawRoundedRectangle(item.bounds, 6.0f, 2.0f);
    } else {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.drawRoundedRectangle(item.bounds, 6.0f, 1.0f);
    }

    // --- Draw Layers (Zebra Shading) ---
    if (item.riff->layers > 1) {
      const float totalH = item.bounds.getHeight();
      const float layerH = totalH / static_cast<float>(item.riff->layers);

      // Clip to item bounds to keep rounding clean
      juce::Graphics::ScopedSaveState signalSave(g);
      g.reduceClipRegion(item.bounds.toNearestInt());

      for (int i = 0; i < item.riff->layers; ++i) {
        auto layerBounds = item.bounds.withHeight(layerH).withY(
            item.bounds.getY() + static_cast<float>(i) * layerH);

        // Alternating fill for zebra effect
        if (i % 2 == 1) {
          g.setColour(juce::Colours::white.withAlpha(0.05f));
          g.fillRect(layerBounds);
        }

        // Horizontal divider line between layers
        if (i > 0) {
          g.setColour(juce::Colours::white.withAlpha(0.1f));
          g.fillRect(layerBounds.getX(), layerBounds.getY(),
                     layerBounds.getWidth(), 1.0f);
        }
      }
    }

    // Mini-waveform - draw over the whole stack
    const float midY = item.bounds.getCentreY();
    const float scaleY = item.bounds.getHeight() * 0.35f;

    g.setColour(isPlaying ? juce::Colours::cyan
                          : juce::Colour(0xFF00CC66).withAlpha(0.6f));

    // Draw waveform - avoid drawing outside item.bounds
    for (int pt = 0; pt < static_cast<int>(item.thumbnail.size()); ++pt) {
      float x = item.bounds.getX() + static_cast<float>(pt);
      float peak =
          std::clamp(item.thumbnail[static_cast<size_t>(pt)], 0.0f, 1.0f);
      g.drawLine(x, midY - peak * scaleY, x, midY + peak * scaleY, 1.0f);
    }

    // Name & Layers (Fitted text with ellipsis to prevent overlap)
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(10.0f));
    juce::String label =
        item.riff->name + " (L" + juce::String(item.riff->layers) + ")";

    // Draw text inside the bounds with some padding
    g.drawFittedText(label, item.bounds.reduced(6.0f, 4.0f).toNearestInt(),
                     juce::Justification::bottomLeft, 1);
  }

  if (items.empty() && riffHistory) {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::italic));
    g.drawText("No riffs yet...", bounds, juce::Justification::centred, false);
  }
}

void RiffHistoryPanel::resized() { updateItems(); }

void RiffHistoryPanel::mouseDown(const juce::MouseEvent &e) {
  for (const auto &item : items) {
    if (item.bounds.contains(e.position)) {
      selectedRiffId = item.riff->id;
      if (onRiffSelected)
        onRiffSelected(*item.riff);
      repaint();
      return;
    }
  }
}
