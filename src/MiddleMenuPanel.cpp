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

void MiddleMenuPanel::setupModeControls(
    juce::Slider &gainSlider, juce::Label &gainLabel,
    juce::Label &gainValueLabel, juce::Slider &bpmSlider, juce::Label &bpmLabel,
    juce::Label &bpmValueLabel, juce::TextButton &monitorButton) {
  pGainSlider = &gainSlider;
  pGainLabel = &gainLabel;
  pGainValueLabel = &gainValueLabel;
  pBpmSlider = &bpmSlider;
  pBpmLabel = &bpmLabel;
  pBpmValueLabel = &bpmValueLabel;
  pMonitorButton = &monitorButton;

  modeContainer.addAndMakeVisible(gainSlider);
  modeContainer.addAndMakeVisible(gainLabel);
  modeContainer.addAndMakeVisible(gainValueLabel);
  modeContainer.addAndMakeVisible(bpmSlider);
  modeContainer.addAndMakeVisible(bpmLabel);
  modeContainer.addAndMakeVisible(bpmValueLabel);
  modeContainer.addAndMakeVisible(monitorButton);

  updateVisibility();
}

void MiddleMenuPanel::setupFxControls(juce::Component &xyPad,
                                      juce::Slider &reverbSizeSlider,
                                      juce::Slider &reverbMixSlider,
                                      juce::Label &reverbSizeLabel,
                                      juce::Label &reverbMixLabel) {
  pXYPad = &xyPad;
  pReverbSizeSlider = &reverbSizeSlider;
  pReverbMixSlider = &reverbMixSlider;
  pReverbSizeLabel = &reverbSizeLabel;
  pReverbMixLabel = &reverbMixLabel;

  fxContainer.addAndMakeVisible(xyPad);
  fxContainer.addAndMakeVisible(reverbSizeSlider);
  fxContainer.addAndMakeVisible(reverbMixSlider);
  fxContainer.addAndMakeVisible(reverbSizeLabel);
  fxContainer.addAndMakeVisible(reverbMixLabel);

  updateVisibility();
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
  if (pGainSlider && pGainLabel && pGainValueLabel && pBpmSlider && pBpmLabel &&
      pBpmValueLabel && pMonitorButton) {
    auto modeArea = modeContainer.getLocalBounds();
    int colW = modeArea.getWidth() / 3;

    // Gain section in left col
    auto gainArea = modeArea.removeFromLeft(colW).reduced(10);
    pGainLabel->setBounds(gainArea.removeFromTop(20));
    pGainValueLabel->setBounds(gainArea.removeFromBottom(20));
    pGainSlider->setBounds(gainArea);

    // BPM section in middle col
    auto bpmArea = modeArea.removeFromLeft(colW).reduced(10);
    pBpmLabel->setBounds(bpmArea.removeFromTop(20));
    pBpmValueLabel->setBounds(bpmArea.removeFromBottom(20));
    pBpmSlider->setBounds(bpmArea);

    // Monitor button in right col
    auto rightArea = modeArea.reduced(20);
    pMonitorButton->setBounds(
        rightArea.withSizeKeepingCentre(rightArea.getWidth(), 40));
  }

  // Layout FX controls
  if (pXYPad && pReverbSizeSlider && pReverbMixSlider && pReverbSizeLabel &&
      pReverbMixLabel) {
    auto fxArea = fxContainer.getLocalBounds().reduced(10);

    // Reserved bottom area for Reverb sliders (80px)
    auto reverbArea = fxArea.removeFromBottom(80);
    auto xyArea = fxArea.reduced(5); // Pad is in the remaining top area

    pXYPad->setBounds(xyArea);

    // Layout Reverb sliders in 2 columns
    int sliderColW = reverbArea.getWidth() / 2;
    auto leftSliderArea = reverbArea.removeFromLeft(sliderColW).reduced(5);
    auto rightSliderArea = reverbArea.reduced(5);

    pReverbSizeLabel->setBounds(leftSliderArea.removeFromTop(20));
    pReverbSizeSlider->setBounds(leftSliderArea);

    pReverbMixLabel->setBounds(rightSliderArea.removeFromTop(20));
    pReverbMixSlider->setBounds(rightSliderArea);
  }
}
