#include "RiffHistoryPanel.h"
#include <algorithm>

RiffHistoryPanel::RiffHistoryPanel() : content(*this) {
  setOpaque(true);
  addAndMakeVisible(viewport);
  viewport.setViewedComponent(&content);
  viewport.setScrollBarsShown(
      false, true, false,
      false); // Horizontal only, no scrollbars shown (cleaner)
}

void RiffHistoryPanel::setHistory(const RiffHistory *history) {
  riffHistory = history;
  content.updateItems();
  repaint();
}

void RiffHistoryPanel::paint(juce::Graphics &g) {
  // Update if history changed
  if (riffHistory) {
    const int currentCount = static_cast<int>(riffHistory->getHistory().size());
    const int currentUpdate = riffHistory->getUpdateCounter();

    if (currentUpdate != lastUpdateCounter || currentCount != lastRiffCount) {
      lastUpdateCounter = currentUpdate;
      lastRiffCount = currentCount;
      content.updateItems();

      // Auto-scroll to the right (newest)
      // Use juce::MessageManager to defer this slightly to ensure component
      // resized? Actually setViewPosition should work here.
      viewport.setViewPosition(
          std::max(0, content.getWidth() - viewport.getWidth()), 0);
    }
  }

  // Background for the whole panel
  g.fillAll(juce::Colour(0xFF0A0A1A));

  // Separator at top
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 1.0f);
}

void RiffHistoryPanel::resized() {
  viewport.setBounds(getLocalBounds());
  content.updateItems();
}

// --- ContentComponent Implementation ---

void RiffHistoryPanel::ContentComponent::updateItems() {
  items.clear();
  if (owner.riffHistory == nullptr) {
    setSize(owner.viewport.getWidth(), owner.viewport.getHeight());
    return;
  }

  const auto &history = owner.riffHistory->getHistory();
  const int numRiffs = static_cast<int>(history.size());

  // Fixed width for each riff box
  const float itemW = 80.0f;
  const float spacing = 10.0f;
  const float margin = 10.0f;

  // Calculate required width
  float requiredW =
      (numRiffs * itemW) + (std::max(0, numRiffs - 1) * spacing) + (2 * margin);

  // Ensure it at least fills the viewport
  setSize(
      static_cast<int>(std::max((float)owner.viewport.getWidth(), requiredW)),
      owner.viewport.getHeight());

  const float h = static_cast<float>(getHeight());

  // Layout from right to left (newest on right)
  // Pin newest to the FAR RIGHT edge of the component (not just requiredW)
  float currentX = (float)getWidth() - margin - itemW;

  for (int i = numRiffs - 1; i >= 0; --i) {
    const auto &riff = history[static_cast<size_t>(i)];
    RiffItem item;
    item.riff = &riff;
    // Remove vertical padding to maximize slot height
    item.bounds = juce::Rectangle<float>(currentX, 0.0f, itemW, h);

    for (const auto &layer : riff.layerBuffers) {
      item.layerThumbnails.push_back(
          generateThumbnail(layer, static_cast<int>(itemW)));
    }

    items.push_back(std::move(item));
    currentX -= (itemW + spacing);
  }

  repaint();
}

void RiffHistoryPanel::ContentComponent::paint(juce::Graphics &g) {
  const auto bounds = getLocalBounds();
  if (bounds.isEmpty())
    return;

  for (const auto &item : items) {
    const bool isSelected = (item.riff->id == owner.selectedRiffId);
    const bool isPlaying =
        owner.isRiffPlaying ? owner.isRiffPlaying(item.riff->id) : false;

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

    // --- Draw Layers (Layer Cake Zebra Shading) ---
    const float totalH = item.bounds.getHeight();
    const float slotH = totalH / 8.0f; // Fixed 8 potential slots

    for (int i = 0; i < static_cast<int>(item.layerThumbnails.size()); ++i) {
      // Build up from bottom
      auto slotBounds = item.bounds.withHeight(slotH).withY(
          item.bounds.getBottom() - static_cast<float>(i + 1) * slotH);

      // Zebra Pattern: 30% brightness difference
      // Background is 0xFF16162B
      // Layer 1 (i=0): 0xFF22223F
      // Layer 2 (i=1): 0xFF353555
      if (i % 2 == 1) {
        g.setColour(juce::Colour(0xFF353555));
      } else {
        g.setColour(juce::Colour(0xFF22223F));
      }
      g.fillRoundedRectangle(slotBounds, 2.0f);

      // Draw per-layer waveform
      g.setColour(isPlaying ? juce::Colours::cyan
                            : juce::Colour(0xFF00CC66).withAlpha(0.6f));

      const auto &thumbnail = item.layerThumbnails[i];
      const float midY = slotBounds.getCentreY();
      const float scaleY = slotH * 0.45f;

      for (int pt = 0; pt < static_cast<int>(thumbnail.size()); ++pt) {
        float x = slotBounds.getX() + static_cast<float>(pt);
        float peak = std::clamp(thumbnail[static_cast<size_t>(pt)], 0.0f, 1.0f);
        g.drawLine(x, midY - peak * scaleY, x, midY + peak * scaleY, 1.0f);
      }

      // Separator line
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.drawHorizontalLine(static_cast<int>(slotBounds.getY()),
                           slotBounds.getX(), slotBounds.getRight());
    }
  }

  if (items.empty() && owner.riffHistory) {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::italic));
    g.drawText("No riffs yet...", bounds, juce::Justification::centred, false);
  }
}

void RiffHistoryPanel::ContentComponent::mouseDown(const juce::MouseEvent &e) {
  for (const auto &item : items) {
    if (item.bounds.contains(e.position.toFloat())) {
      owner.selectedRiffId = item.riff->id;
      if (owner.onRiffSelected)
        owner.onRiffSelected(*item.riff);
      owner.repaint();
      return;
    }
  }

  // Clicked empty space
  owner.selectedRiffId = juce::Uuid();
  owner.repaint();
}

std::vector<float> RiffHistoryPanel::ContentComponent::generateThumbnail(
    const juce::AudioBuffer<float> &audio, int numPoints) {
  std::vector<float> points(static_cast<size_t>(numPoints), 0.0f);
  if (audio.getNumSamples() == 0 || numPoints == 0)
    return points;

  const int numSamples = audio.getNumSamples();
  const int samplesPerPoint = std::max(1, numSamples / numPoints);
  float maxFound = 0.0f;

  for (int i = 0; i < numPoints; ++i) {
    int start = i * samplesPerPoint;
    int end = std::min(start + samplesPerPoint, numSamples);
    float peak = 0.0f;

    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
      const float *data = audio.getReadPointer(ch);
      for (int s = start; s < end; ++s) {
        peak = std::max(peak, std::abs(data[s]));
      }
    }
    points[static_cast<size_t>(i)] = peak;
    maxFound = std::max(maxFound, peak);
  }

  // Normalization to make waveforms fill the slot (tiny height fix)
  if (maxFound > 0.001f) {
    for (auto &p : points) {
      p /= maxFound;
    }
  }

  return points;
}
