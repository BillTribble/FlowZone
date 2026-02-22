#include "MiddleMenuPanel.h"

MiddleMenuPanel::MiddleMenuPanel() {
  setBufferedToImage(true);
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

  // setupMixerControls(); // Removed for V9 refactor
  // updateVisibility();
}

void MiddleMenuPanel::setupMicReverb() {
  micReverbRoomSizeLabel.setText("REV SIZE", juce::dontSendNotification);
  micReverbRoomSizeLabel.setJustificationType(juce::Justification::centred);
  micReverbRoomSizeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  addAndMakeVisible(micReverbRoomSizeLabel);

  micReverbWetLevelLabel.setText("REV MIX", juce::dontSendNotification);
  micReverbWetLevelLabel.setJustificationType(juce::Justification::centred);
  micReverbWetLevelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  addAndMakeVisible(micReverbWetLevelLabel);
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

  int tabCols = 3;
  int tabW = tabsArea.getWidth() / tabCols;
  modeTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  fxTabButton.setBounds(tabsArea.removeFromLeft(tabW).reduced(2, 5));
  mixerTabButton.setBounds(tabsArea.reduced(2, 5));
}

// Layout moved to Panels in V9
