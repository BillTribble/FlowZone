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
  setupTabButton(mixerTabButton, "MIXER");

  modeTabButton.setToggleState(true, juce::dontSendNotification);

  modeTabButton.onClick = [this]() { setActiveTab(Tab::Mode); };
  fxTabButton.onClick = [this]() { setActiveTab(Tab::FX); };
  mixerTabButton.onClick = [this]() { setActiveTab(Tab::Mixer); };

  addAndMakeVisible(modeContainer);
  addAndMakeVisible(fxContainer);
  addAndMakeVisible(mixerContainer);

  setupMixerControls();
  updateVisibility();
}

void MiddleMenuPanel::setupModeControls(LabeledKnob &gainKnob,
                                        juce::TextButton &monitorButton) {
  pGainKnob = &gainKnob;
  pMonitorButton = &monitorButton;

  modeContainer.addAndMakeVisible(gainKnob);
  modeContainer.addAndMakeVisible(monitorButton);

  updateVisibility();
}

void MiddleMenuPanel::setupMicReverb(juce::Slider &roomSize,
                                     juce::Slider &wetLevel) {
  pMicReverbRoomSize = &roomSize;
  pMicReverbWetLevel = &wetLevel;

  modeContainer.addAndMakeVisible(roomSize);
  modeContainer.addAndMakeVisible(wetLevel);

  micReverbRoomSizeLabel.setText("REV SIZE", juce::dontSendNotification);
  micReverbRoomSizeLabel.setJustificationType(juce::Justification::centred);
  micReverbRoomSizeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  modeContainer.addAndMakeVisible(micReverbRoomSizeLabel);

  micReverbWetLevelLabel.setText("REV MIX", juce::dontSendNotification);
  micReverbWetLevelLabel.setJustificationType(juce::Justification::centred);
  micReverbWetLevelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  modeContainer.addAndMakeVisible(micReverbWetLevelLabel);

  updateVisibility();
}

void MiddleMenuPanel::setupFxControls(XYPad &xyPad,
                                      juce::Slider &reverbSizeSlider,
                                      juce::Label &reverbSizeLabel) {
  pXYPad = &xyPad;
  pReverbSizeSlider = &reverbSizeSlider;
  pReverbSizeLabel = &reverbSizeLabel;

  fxContainer.addAndMakeVisible(xyPad);
  fxContainer.addAndMakeVisible(reverbSizeSlider);
  fxContainer.addAndMakeVisible(reverbSizeLabel);

  updateVisibility();
}

void MiddleMenuPanel::setupMixerControls() {
  auto setupToggle = [this](juce::TextButton &b) {
    b.setClickingTogglesState(true);
    b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A2E));
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00CC66));
    b.setColour(juce::TextButton::textColourOffId,
                juce::Colours::white.withAlpha(0.6f));
    b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    mixerContainer.addAndMakeVisible(b);
  };

  setupToggle(snapToggle);
  setupToggle(autoQuantizeToggle);

  mixerPlaceholderLabel.setText("LOOPER BEHAVIOR", juce::dontSendNotification);
  mixerPlaceholderLabel.setJustificationType(juce::Justification::centred);
  mixerPlaceholderLabel.setColour(juce::Label::textColourId,
                                  juce::Colours::white.withAlpha(0.5f));
  mixerPlaceholderLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  mixerContainer.addAndMakeVisible(mixerPlaceholderLabel);
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
  mixerContainer.setVisible(activeTab == Tab::Mixer);
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

  int tabW = tabsArea.getWidth() / 3;
  modeTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  fxTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  mixerTabButton.setBounds(tabsArea.reduced(2, 5));

  modeContainer.setBounds(area);
  fxContainer.setBounds(area);
  mixerContainer.setBounds(area);

  // Layout MIXER controls
  auto mixerArea = mixerContainer.getLocalBounds().reduced(20);
  mixerPlaceholderLabel.setBounds(mixerArea.removeFromTop(30));

  auto toggleRow = mixerArea.removeFromTop(40);
  snapToggle.setBounds(
      toggleRow.removeFromLeft(mixerArea.getWidth() / 2).reduced(5));
  autoQuantizeToggle.setBounds(toggleRow.reduced(5));

  // Layout MODE controls
  if (pGainKnob && pMonitorButton) {
    auto modeArea = modeContainer.getLocalBounds().reduced(10);

    // Three columns: Gain, Mic Reverb, Monitor
    int colW = modeArea.getWidth() / 3;
    pGainKnob->setBounds(modeArea.removeFromLeft(colW).reduced(2));

    auto reverbCol = modeArea.removeFromLeft(colW).reduced(5);
    micReverbRoomSizeLabel.setBounds(reverbCol.removeFromTop(20));
    if (pMicReverbRoomSize)
      pMicReverbRoomSize->setBounds(reverbCol.removeFromTop(50));
    reverbCol.removeFromTop(10); // spacing
    micReverbWetLevelLabel.setBounds(reverbCol.removeFromTop(20));
    if (pMicReverbWetLevel)
      pMicReverbWetLevel->setBounds(reverbCol.removeFromTop(50));

    pMonitorButton->setBounds(modeArea.reduced(2, 20));
  }

  // Layout FX controls
  if (pXYPad && pReverbSizeSlider && pReverbSizeLabel) {
    auto fxArea = fxContainer.getLocalBounds().reduced(10);

    // Reserved bottom area for Reverb slider (60px)
    auto reverbArea = fxArea.removeFromBottom(60);
    auto xyArea = fxArea.reduced(5);

    pXYPad->setBounds(xyArea);

    auto reverbLabelArea = reverbArea.removeFromLeft(50);
    pReverbSizeLabel->setBounds(reverbLabelArea);
    pReverbSizeSlider->setBounds(reverbArea.reduced(5));
  }
}
