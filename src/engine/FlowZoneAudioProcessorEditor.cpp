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
}

FlowZoneAudioProcessorEditor::~FlowZoneAudioProcessorEditor() {}

void FlowZoneAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void FlowZoneAudioProcessorEditor::resized() {
  webView.setBounds(getLocalBounds());
}
