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
    // 2. Heuristic: in development, we usually have the src directory
    // We'll check for a "DEBUG" flag or just try the dev server
#if DEBUG
    urlToLoad = "http://localhost:5173";
#else
    // In production, we load from the bundled/relative dist folder
    auto executable =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto projectDir = executable.getParentDirectory()
                          .getParentDirectory()
                          .getParentDirectory(); // Adjust based on build layout
    auto distFile = projectDir.getChildFile("src/web_client/dist/index.html");

    if (distFile.existsAsFile()) {
      urlToLoad = "file://" + distFile.getFullPathName();
    } else {
      // Last resort fallback to dev server if file not found
      urlToLoad = "http://localhost:5173";
    }
#endif
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
