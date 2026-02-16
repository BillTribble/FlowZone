#include "FlowZoneAudioProcessorEditor.h"
#include "FlowZoneAudioProcessor.h"

FlowZoneAudioProcessorEditor::FlowZoneAudioProcessorEditor(
    FlowZoneAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  addAndMakeVisible(webView);

  // In development, point to the Vite dev server
  // In production, this would point to a local file or embedded resource
  webView.goToURL("http://localhost:5173");

  setSize(1200, 800);
  setResizable(true, true); // Enable window resizing
  setResizeLimits(800, 600, 3840, 2160); // Min: 800x600, Max: 4K
}

FlowZoneAudioProcessorEditor::~FlowZoneAudioProcessorEditor() {}

void FlowZoneAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void FlowZoneAudioProcessorEditor::resized() {
  webView.setBounds(getLocalBounds());
}
