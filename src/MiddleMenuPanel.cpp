#include "MiddleMenuPanel.h"

MiddleMenuPanel::MiddleMenuPanel() {
  auto setupTabButton = [this](juce::TextButton &b, const juce::String &name) {
    b.setButtonText(name);
    b.setClickingTogglesState(true);
    b.setRadioGroupId(1234); // Same group for radio behavior
    b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A2E));
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00CC66));
    b.setColour(juce::TextButton::textColourOffId,
                juce::Colours::white.withAlpha(0.6f));
    b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible(b);
  };

  setupTabButton(modeTabButton, "MODE");
  setupTabButton(fxTabButton, "FX");

  modeTabButton.setToggleState(true, juce::dontSendNotification);

  modeTabButton.onClick = [this]() { setActiveTab(Tab::Mode); };
  fxTabButton.onClick = [this]() { setActiveTab(Tab::FX); };

  addAndMakeVisible(modeContainer);
  addAndMakeVisible(fxContainer);

  updateVisibility();
}

void MiddleMenuPanel::setupModeControls(juce::Slider &gainSlider,
                                        juce::Label &gainLabel,
                                        juce::Label &gainValueLabel,
                                        juce::TextButton &monitorButton) {
  pGainSlider = &gainSlider;
  pGainLabel = &gainLabel;
  pGainValueLabel = &gainValueLabel;
  pMonitorButton = &monitorButton;

  modeContainer.addAndMakeVisible(gainSlider);
  modeContainer.addAndMakeVisible(gainLabel);
  modeContainer.addAndMakeVisible(gainValueLabel);
  modeContainer.addAndMakeVisible(monitorButton);
}

void MiddleMenuPanel::setupFxControls(juce::Component &xyPad) {
  pXYPad = &xyPad;
  fxContainer.addAndMakeVisible(xyPad);
}

void MiddleMenuPanel::setActiveTab(Tab tab) {
  if (activeTab == tab)
    return;
  activeTab = tab;
  updateVisibility();
}

void MiddleMenuPanel::updateVisibility() {
  modeContainer.setVisible(activeTab == Tab::Mode);
  fxContainer.setVisible(activeTab == Tab::FX);
}

void MiddleMenuPanel::paint(juce::Graphics &g) {
  // Container background
  auto area = getLocalBounds().toFloat();
  auto contentArea = area.withTrimmedTop(40.0f);

  g.setColour(juce::Colour(0xFF16162B));
  g.fillRoundedRectangle(contentArea, 6.0f);

  g.setColour(juce::Colours::white.withAlpha(0.05f));
  g.drawRoundedRectangle(contentArea, 6.0f, 1.0f);

  if (activeTab == Tab::FX) {
    // XY Pad is drawn by child component
  }
}

void MiddleMenuPanel::resized() {
  auto area = getLocalBounds();
  auto tabsArea = area.removeFromTop(40);

  int tabW = tabsArea.getWidth() / 2;
  modeTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  fxTabButton.setBounds(tabsArea.reduced(2, 5));

  auto contentArea = area.reduced(10);
  modeContainer.setBounds(contentArea);
  fxContainer.setBounds(contentArea);

  // Layout mode controls if they are set up
  if (pGainSlider && pGainLabel && pGainValueLabel && pMonitorButton) {
    auto modeArea = modeContainer.getLocalBounds();
    auto leftCol = modeArea.removeFromLeft(modeArea.getWidth() / 2);

    // Gain section in left col
    auto gainArea = leftCol.reduced(10);
    pGainLabel->setBounds(gainArea.removeFromTop(20));
    pGainValueLabel->setBounds(gainArea.removeFromBottom(20));
    pGainSlider->setBounds(gainArea);

    // Monitor button in right col
    auto rightCol = modeArea.reduced(20);
    pMonitorButton->setBounds(
        rightCol.withSizeKeepingCentre(rightCol.getWidth(), 40));
  }

  // Layout FX controls
  if (pXYPad) {
    auto fxArea = fxContainer.getLocalBounds().reduced(10);
    pXYPad->setBounds(fxArea);
  }
}
