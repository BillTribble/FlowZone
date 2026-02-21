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
  setupTabButton(soundTabButton, "SOUND");
  setupTabButton(fxTabButton, "FX");
  setupTabButton(mixerTabButton, "MIXER");

  modeTabButton.setToggleState(true, juce::dontSendNotification);

  modeTabButton.onClick = [this]() { setActiveTab(Tab::Mode); };
  soundTabButton.onClick = [this]() { setActiveTab(Tab::Sound); };
  fxTabButton.onClick = [this]() { setActiveTab(Tab::FX); };
  mixerTabButton.onClick = [this]() { setActiveTab(Tab::Mixer); };

  // setupMixerControls(); // Removed for V9 refactor
  // updateVisibility();
}

void MiddleMenuPanel::setupModeControls(LabeledKnob &gainKnob,
                                        juce::TextButton &monitorButton) {
  pGainKnob = &gainKnob;
  pMonitorButton = &monitorButton;

  addAndMakeVisible(gainKnob);
  addAndMakeVisible(monitorButton);

  updateVisibility();
}

void MiddleMenuPanel::setupMicReverb(juce::Slider &roomSize,
                                     juce::Slider &wetLevel) {
  pMicReverbRoomSize = &roomSize;
  pMicReverbWetLevel = &wetLevel;

  addAndMakeVisible(roomSize);
  addAndMakeVisible(wetLevel);

  micReverbRoomSizeLabel.setText("REV SIZE", juce::dontSendNotification);
  micReverbRoomSizeLabel.setJustificationType(juce::Justification::centred);
  micReverbRoomSizeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  addAndMakeVisible(micReverbRoomSizeLabel);
  addAndMakeVisible(micReverbRoomSizeLabel);

  micReverbWetLevelLabel.setText("REV MIX", juce::dontSendNotification);
  micReverbWetLevelLabel.setJustificationType(juce::Justification::centred);
  micReverbWetLevelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  addAndMakeVisible(micReverbWetLevelLabel);
}

void MiddleMenuPanel::setupFxControls() {
  // Logic moved to Panels in V9
}

void MiddleMenuPanel::setupMixerControls() {
  // Mixer logic to be moved to Mixer Panel in V9
}

void MiddleMenuPanel::setActiveTab(Tab tab) {
  if (activeTab == tab)
    return;
  activeTab = tab;
  if (onTabChanged)
    onTabChanged(tab);
  updateVisibility();
}

void MiddleMenuPanel::updateVisibility() {
  // Panels are now managed by MainComponent
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

  int tabW = tabsArea.getWidth() / 4;
  modeTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  soundTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  fxTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  mixerTabButton.setBounds(tabsArea.reduced(2, 5));
}

// Layout moved to Panels in V9
