#include "XYPad.h"

XYPad::XYPad() { setOpaque(true); }

void XYPad::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(juce::Colour(0xFF1A1A2E));
  g.fillRoundedRectangle(bounds, 8.0f);

  // Grid lines
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  for (float i = 0.25f; i < 1.0f; i += 0.25f) {
    g.drawVerticalLine(static_cast<int>(bounds.getWidth() * i), 0.0f,
                       bounds.getHeight());
    g.drawHorizontalLine(static_cast<int>(bounds.getHeight() * i), 0.0f,
                         bounds.getWidth());
  }

  // Border
  g.setColour(juce::Colour(0xFF2A2A4A));
  g.drawRoundedRectangle(bounds, 8.0f, 2.0f);

  // The "Pad" area reduced slightly
  auto padArea = bounds.reduced(10.0f);

  // Dot position
  float dotX = padArea.getX() + (xValue * padArea.getWidth());
  float dotY = padArea.getY() + ((1.0f - yValue) * padArea.getHeight());

  // Only draw indicator if mouse is held down
  if (isMouseButtonDown()) {
    // Dot shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(dotX - 8.0f, dotY - 8.0f, 18.0f, 18.0f);

    // The Dot
    g.setColour(juce::Colour(0xFF00CC66));
    g.fillEllipse(dotX - 10.0f, dotY - 10.0f, 20.0f, 20.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(dotX - 10.0f, dotY - 10.0f, 20.0f, 20.0f, 2.0f);
  }
}

void XYPad::mouseDown(const juce::MouseEvent &e) { handleMouse(e); }

void XYPad::mouseDrag(const juce::MouseEvent &e) { handleMouse(e); }

void XYPad::mouseUp(const juce::MouseEvent &e) {
  if (onRelease)
    onRelease();
}

void XYPad::handleMouse(const juce::MouseEvent &e) {
  auto padArea = getLocalBounds().toFloat().reduced(10.0f);

  xValue = juce::jlimit(0.0f, 1.0f,
                        (e.position.x - padArea.getX()) / padArea.getWidth());
  yValue = juce::jlimit(
      0.0f, 1.0f, 1.0f - (e.position.y - padArea.getY()) / padArea.getHeight());

  if (onXYChange)
    onXYChange(xValue, yValue);

  repaint();
}

void XYPad::setValues(float x, float y) {
  xValue = juce::jlimit(0.0f, 1.0f, x);
  yValue = juce::jlimit(0.0f, 1.0f, y);
  repaint();
}

void XYPad::resized() {}
