#include "FlowZoneAudioProcessorEditor.h"
#include "FlowZoneAudioProcessor.h"

FlowZoneAudioProcessorEditor::FlowZoneAudioProcessorEditor(
    FlowZoneAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  addAndMakeVisible(webView);

  // Determine which URL to load
  juce::String urlToLoad;

  // 1. Try environment variable first (for flexible dev setups)
  auto envUrl =
      juce::SystemStats::getEnvironmentVariable("FLOWZONE_UI_URL", "");
  if (envUrl.isNotEmpty()) {
    urlToLoad = envUrl;
  } else {
    // 2. Default to our embedded CivetWeb server on 50001
    // This server is configured in FlowZoneAudioProcessor to point to the local
    // dist folder
    urlToLoad = "http://localhost:50001";
  }

  webView.goToURL(urlToLoad);

  setSize(1200, 800);
  setResizable(true, true);              // Enable window resizing
  setResizeLimits(800, 600, 3840, 2160); // Min: 800x600, Max: 4K
}

FlowZoneAudioProcessorEditor::~FlowZoneAudioProcessorEditor() {}

void FlowZoneAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void FlowZoneAudioProcessorEditor::resized() {
  webView.setBounds(getLocalBounds());
}
