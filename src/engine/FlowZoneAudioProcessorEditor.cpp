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
    // 2. Search for the local dist/index.html file
    // We'll check several potential relative paths to find the project root
    auto currentFile =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File distFile;

    // Try up to 8 levels up (to cover standard build structures and bundles)
    auto searchDir = currentFile.getParentDirectory();
    for (int i = 0; i < 8; ++i) {
      auto potentialDist =
          searchDir.getChildFile("src/web_client/dist/index.html");
      if (potentialDist.existsAsFile()) {
        distFile = potentialDist;
        break;
      }
      searchDir = searchDir.getParentDirectory();
    }

    if (distFile.existsAsFile()) {
      urlToLoad = "file://" + distFile.getFullPathName();
    } else {
      // 3. Last resort fallback to dev server
      urlToLoad = "http://localhost:5173";
    }
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
