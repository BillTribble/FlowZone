#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class MixerTabTop : public juce::Component {
public:
  MixerTabTop() {
    addAndMakeVisible(quantizeBtn);
    quantizeBtn.setButtonText("Quantize: OFF");
    quantizeBtn.setClickingTogglesState(true);
    quantizeBtn.onClick = [this] {
      quantizeBtn.setButtonText(quantizeBtn.getToggleState() ? "Quantize: ON"
                                                             : "Quantize: OFF");
      if (onQuantizeToggled)
        onQuantizeToggled(quantizeBtn.getToggleState());
    };

    addAndMakeVisible(metronomeBtn);
    metronomeBtn.setButtonText("Metronome: OFF");
    metronomeBtn.setClickingTogglesState(true);
    metronomeBtn.onClick = [this] {
      metronomeBtn.setButtonText(
          metronomeBtn.getToggleState() ? "Metronome: ON" : "Metronome: OFF");
      if (onMetronomeToggled)
        onMetronomeToggled(metronomeBtn.getToggleState());
    };

    addAndMakeVisible(moreBtn);
    moreBtn.setButtonText("Settings");
    moreBtn.onClick = [this] {
      if (onMoreClicked)
        onMoreClicked();
    };

    auto styleBtn = [](juce::TextButton &btn) {
      btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A2E));
      btn.setColour(juce::TextButton::buttonOnColourId,
                    juce::Colour(0xFF00CC66));
      btn.setColour(juce::TextButton::textColourOffId,
                    juce::Colours::white.withAlpha(0.7f));
      btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    };

    styleBtn(quantizeBtn);
    styleBtn(metronomeBtn);
    styleBtn(moreBtn);
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(20);
    int h = bounds.getHeight();
    quantizeBtn.setBounds(
        bounds.removeFromLeft(120).withSizeKeepingCentre(120, 30));
    bounds.removeFromLeft(10);
    metronomeBtn.setBounds(
        bounds.removeFromLeft(120).withSizeKeepingCentre(120, 30));
    bounds.removeFromLeft(10);
    moreBtn.setBounds(bounds.removeFromLeft(80).withSizeKeepingCentre(80, 30));
  }

  std::function<void(bool)> onQuantizeToggled;
  std::function<void(bool)> onMetronomeToggled;
  std::function<void()> onMoreClicked;

private:
  juce::TextButton quantizeBtn;
  juce::TextButton metronomeBtn;
  juce::TextButton moreBtn;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTabTop)
};

#include "LevelMeter.h"

class MixChannel : public juce::Component {
public:
  MixChannel(int index) : channelIndex(index) {
    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M" + juce::String(index + 1));
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonColourId,
                      juce::Colour(0xFF2A2A3A));
    muteBtn.setColour(juce::TextButton::buttonOnColourId,
                      juce::Colours::red.withAlpha(0.6f));
    muteBtn.onClick = [this] {
      if (onMuteChanged)
        onMuteChanged(channelIndex, muteBtn.getToggleState());
    };

    addAndMakeVisible(fader);
    fader.setSliderStyle(juce::Slider::LinearHorizontal);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setRange(0.0, 1.0);
    fader.setValue(1.0);
    fader.onValueChange = [this] {
      if (onVolumeChanged)
        onVolumeChanged(channelIndex, (float)fader.getValue());
    };

    addAndMakeVisible(meter);
    meter.setHorizontal(true);
  }

  void resized() override {
    auto bounds = getLocalBounds();
    muteBtn.setBounds(bounds.removeFromLeft(35).reduced(2));

    auto meterBounds = bounds.removeFromBottom(bounds.getHeight() / 3);
    meter.setBounds(meterBounds.reduced(2));
    fader.setBounds(bounds.reduced(2));
  }

  void setLevel(float level) { meter.setLevel(level); }

  std::function<void(int, bool)> onMuteChanged;
  std::function<void(int, float)> onVolumeChanged;

private:
  int channelIndex;
  juce::TextButton muteBtn;
  juce::Slider fader;
  LevelMeter meter;
};

class MixerTabBottom : public juce::Component {
public:
  MixerTabBottom() {
    for (int i = 0; i < 8; ++i) {
      auto *ch = channels.add(new MixChannel(i));
      addAndMakeVisible(ch);

      ch->onMuteChanged = [this](int idx, bool muted) {
        if (onChannelMute)
          onChannelMute(idx, muted);
      };
      ch->onVolumeChanged = [this](int idx, float vol) {
        if (onChannelVolume)
          onChannelVolume(idx, vol);
      };
    }

    addAndMakeVisible(commitBtn);
    commitBtn.setButtonText("Commit Mix");
    commitBtn.setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0xFF00CC66));
    commitBtn.setColour(juce::TextButton::textColourOffId,
                        juce::Colours::black);
    commitBtn.onClick = [this] {
      if (onCommit)
        onCommit();
    };
  }

  void resized() override {
    auto area = getLocalBounds().reduced(5);
    commitBtn.setBounds(area.removeFromBottom(30).reduced(40, 2));

    int rowHeight = area.getHeight() / 8;

    for (int i = 0; i < 8; ++i) {
      auto row = area.removeFromTop(rowHeight);
      if (i % 2 != 0) {
        row.removeFromRight(20);
      } else {
        row.removeFromLeft(20);
      }
      channels[i]->setBounds(row.reduced(2));
    }
  }

  void setChannelLevel(int index, float level) {
    if (index >= 0 && index < channels.size())
      channels[index]->setLevel(level);
  }

  std::function<void(int, bool)> onChannelMute;
  std::function<void(int, float)> onChannelVolume;
  std::function<void()> onCommit;

private:
  juce::OwnedArray<MixChannel> channels;
  juce::TextButton commitBtn;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTabBottom)
};
