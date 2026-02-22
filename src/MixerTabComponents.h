#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class LoopLengthSelector : public juce::Component {
public:
  LoopLengthSelector() {
    addAndMakeVisible(label);
    label.setText("Loop lengths", juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId,
                    juce::Colours::white.withAlpha(0.6f));

    const std::vector<float> lengths = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f,
                                        5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    for (float len : lengths) {
      juce::String text = juce::String(len, 1).replace(".0", "");
      auto *btn = btns.add(new juce::TextButton(text));
      addAndMakeVisible(btn);
      btn->setClickingTogglesState(true);
      btn->setColour(juce::TextButton::buttonColourId,
                     juce::Colour(0xFF1A1A2E));
      btn->setColour(juce::TextButton::buttonOnColourId,
                     juce::Colour(0xFF00CC66));

      if (len == 1.0f || len == 2.0f || len == 4.0f || len == 8.0f) {
        btn->setToggleState(true, juce::dontSendNotification);
      }

      btn->onClick = [this] {
        if (onSelectionChanged)
          onSelectionChanged(getActiveLengths());
      };
    }
  }

  void resized() override {
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(20));

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    int w = getWidth() / 5;
    for (int i = 0; i < 5; ++i) {
      btns[i]->setBounds(row1.removeFromLeft(w).reduced(1));
      btns[i + 5]->setBounds(row2.removeFromLeft(w).reduced(1));
    }
  }

  std::vector<float> getActiveLengths() const {
    std::vector<float> active;
    const std::vector<float> lengths = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f,
                                        5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    for (int i = 0; i < btns.size(); ++i) {
      if (btns[i]->getToggleState())
        active.push_back(lengths[i]);
    }
    return active;
  }

  std::function<void(std::vector<float>)> onSelectionChanged;

private:
  juce::Label label;
  juce::OwnedArray<juce::TextButton> btns;
};

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

    addAndMakeVisible(loopSelector);
    loopSelector.onSelectionChanged = [this](std::vector<float> active) {
      if (onLengthsChanged)
        onLengthsChanged(active);
    };
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(10);
    int h = bounds.getHeight();
    quantizeBtn.setBounds(
        bounds.removeFromLeft(120).withSizeKeepingCentre(120, 30));
    bounds.removeFromLeft(10);
    metronomeBtn.setBounds(
        bounds.removeFromLeft(120).withSizeKeepingCentre(120, 30));
    bounds.removeFromLeft(10);
    moreBtn.setBounds(bounds.removeFromLeft(80).withSizeKeepingCentre(80, 30));

    bounds.removeFromLeft(20);
    loopSelector.setBounds(bounds);
  }

  std::function<void(bool)> onQuantizeToggled;
  std::function<void(bool)> onMetronomeToggled;
  std::function<void()> onMoreClicked;
  std::function<void(std::vector<float>)> onLengthsChanged;

private:
  juce::TextButton quantizeBtn;
  juce::TextButton metronomeBtn;
  juce::TextButton moreBtn;
  LoopLengthSelector loopSelector;

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
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setRange(0.0, 1.0);
    fader.setValue(1.0);
    fader.onValueChange = [this] {
      if (onVolumeChanged)
        onVolumeChanged(channelIndex, (float)fader.getValue());
    };

    addAndMakeVisible(meter);
    meter.setHorizontal(false);
  }

  void resized() override {
    auto bounds = getLocalBounds();
    muteBtn.setBounds(bounds.removeFromBottom(25).reduced(2));

    auto meterBounds = bounds.removeFromRight(bounds.getWidth() / 3);
    meter.setBounds(meterBounds.reduced(2));
    fader.setBounds(bounds.reduced(2));
  }

  void setLayerActive(bool active) {
    if (isActive == active)
      return;
    isActive = active;
    fader.setEnabled(active);
    fader.setAlpha(active ? 1.0f : 0.3f);
    muteBtn.setEnabled(active);
    muteBtn.setAlpha(active ? 1.0f : 0.3f);
    meter.setAlpha(active ? 1.0f : 0.3f);
    repaint();
  }

  void setLevel(float level) { meter.setLevel(level); }

  std::function<void(int, bool)> onMuteChanged;
  std::function<void(int, float)> onVolumeChanged;

private:
  int channelIndex;
  bool isActive{true};
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

    int colWidth = area.getWidth() / 8;

    for (int i = 0; i < 8; ++i) {
      auto col = area.removeFromLeft(colWidth);
      channels[i]->setBounds(col.reduced(2));
    }
  }

  void setChannelLevel(int index, float level) {
    if (index >= 0 && index < channels.size())
      channels[index]->setLevel(level);
  }

  void setLayerActive(int index, bool active) {
    if (index >= 0 && index < channels.size())
      channels[index]->setLayerActive(active);
  }

  std::function<void(int, bool)> onChannelMute;
  std::function<void(int, float)> onChannelVolume;
  std::function<void()> onCommit;

private:
  juce::OwnedArray<MixChannel> channels;
  juce::TextButton commitBtn;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTabBottom)
};
