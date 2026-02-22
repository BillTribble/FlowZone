#pragma once
#include "Riff.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

/**
 * A horizontal row showing the history of captured riffs.
 * Riffs flow from right to left (newest on the right).
 */
class RiffHistoryPanel : public juce::Component, private juce::Timer {
public:
  RiffHistoryPanel();
  ~RiffHistoryPanel() override = default;

  void setHistory(const RiffHistory *history);

  void paint(juce::Graphics &g) override;
  void resized() override;

  std::function<void(const Riff &)> onRiffSelected;
  std::function<bool(const juce::Uuid &)> isRiffPlaying;
  juce::Uuid getSelectedRiffId() const { return selectedRiffId; }

private:
  void timerCallback() override;

  struct RiffItem {
    juce::Uuid riffId;
    juce::Rectangle<float> currentBounds; // Animated position
    juce::Rectangle<float> targetBounds;  // Where it SHOULD be
    std::vector<std::vector<float>> layerThumbnails;
  };

  /** Internal component that actually draws the riffs and is scrolled by the
   * viewport. */
  class ContentComponent : public juce::Component, private juce::Timer {
  public:
    ContentComponent(RiffHistoryPanel &p) : owner(p) {}
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void updateItems();

  private:
    void timerCallback() override;

    std::vector<RiffItem> items;
    int dragStartX{0};
    int dragStartScrollX{0};

  private:
    RiffHistoryPanel &owner;
    std::vector<float> generateThumbnail(const juce::AudioBuffer<float> &audio,
                                         int numPoints);
  };

  juce::Viewport viewport;
  ContentComponent content;

  const RiffHistory *riffHistory{nullptr};
  juce::Uuid selectedRiffId;
  int lastUpdateCounter{-1};
  int lastRiffCount{0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RiffHistoryPanel)
};
