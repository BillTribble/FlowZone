#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A simple XY pad for controlling two parameters.
 * X and Y both range from 0.0 to 1.0.
 */
class XYPad : public juce::Component {
public:
  XYPad();
  ~XYPad() override = default;

  void paint(juce::Graphics &g) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent &e) override;

  std::function<void(float, float)> onXYChange;
  std::function<void()> onRelease;

  void setValues(float x, float y);
  float getXValue() const { return xValue; }
  float getYValue() const { return yValue; }

private:
  void handleMouse(const juce::MouseEvent &e);

  float xValue{0.5f};
  float yValue{0.5f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};
