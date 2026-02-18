#include "FlowZoneAudioProcessor.h"
#include "FlowZoneAudioProcessorEditor.h"

#ifndef JucePlugin_PreferredChannelConfigurations
FlowZoneAudioProcessor::FlowZoneAudioProcessor()
    : juce::AudioProcessor(
          juce::AudioProcessor::BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withInput("Extra Inputs",
                         juce::AudioChannelSet::discreteChannels(14), false)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withOutput("Out 3-4", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 5-6", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 7-8", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 9-10", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 11-12", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 13-14", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 15-16", juce::AudioChannelSet::stereo(), false)),
      engine(), server(50001) {}
#else
FlowZoneAudioProcessor::FlowZoneAudioProcessor() : engine(), server(50001) {}
#endif

// Separated initialization logic to constructor body
void FlowZoneAudioProcessor::init() {
  // Discover project root and set document root for the server
  auto currentFile =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile);
  juce::File projectRoot;
  auto searchDir = currentFile.getParentDirectory();
  for (int i = 0; i < 8; ++i) {
    if (searchDir.getChildFile("FlowZone.jucer").existsAsFile()) {
      projectRoot = searchDir;
      break;
    }
    searchDir = searchDir.getParentDirectory();
  }

  if (projectRoot.isDirectory()) {
    auto distDir = projectRoot.getChildFile("src/web_client/dist");
    if (distDir.isDirectory()) {
      server.setDocumentRoot(distDir.getFullPathName().toStdString());
    }
  }

  // Mark application as active (crash detection)
  crashGuard.markActive();

  // Set up WebSocket -> CommandQueue flow
  server.setOnMessageCallback([this](const std::string &msg) {
    juce::String juceMsg(msg);
    engine.getCommandQueue().push(juceMsg);
  });

  // Set up StateBroadcaster -> WebSocket broadcast flow
  engine.getBroadcaster().setMessageCallback(
      [this](const juce::String &msg) { server.broadcast(msg.toStdString()); });

  // Set up initial state callback for new connections
  server.setInitialStateCallback([this]() -> std::string {
    auto state = engine.getSessionManager().getCurrentState();
    state.transport.isPlaying = engine.getTransport().isPlaying();
    state.transport.bpm = engine.getTransport().getBpm();
    state.transport.metronomeEnabled =
        engine.getTransport().isMetronomeEnabled();
    state.transport.loopLengthBars = engine.getTransport().getLoopLengthBars();
    state.transport.barPhase = engine.getTransport().getBarPhase();

    juce::DynamicObject *root = new juce::DynamicObject();
    root->setProperty("type", "STATE_FULL");
    root->setProperty("revisionId",
                      (juce::int64)engine.getBroadcaster().getRevisionId());
    root->setProperty("data", state.toVar());

    return juce::JSON::toString(juce::var(root)).toStdString();
  });

  server.start();
}

FlowZoneAudioProcessor::~FlowZoneAudioProcessor() { crashGuard.markClean(); }

const juce::String FlowZoneAudioProcessor::getName() const {
  return JucePlugin_Name;
}
bool FlowZoneAudioProcessor::acceptsMidi() const { return true; }
bool FlowZoneAudioProcessor::producesMidi() const { return true; }
bool FlowZoneAudioProcessor::isMidiEffect() const { return false; }
double FlowZoneAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int FlowZoneAudioProcessor::getNumPrograms() { return 1; }
int FlowZoneAudioProcessor::getCurrentProgram() { return 0; }
void FlowZoneAudioProcessor::setCurrentProgram(int index) {}
const juce::String FlowZoneAudioProcessor::getProgramName(int index) {
  return {};
}
void FlowZoneAudioProcessor::changeProgramName(int index,
                                               const juce::String &newName) {}

void FlowZoneAudioProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock) {
  // Request Microphone permission
  juce::RuntimePermissions::request(
      juce::RuntimePermissions::recordAudio,
      [this, sampleRate, samplesPerBlock](bool granted) {
        engine.prepareToPlay(sampleRate, samplesPerBlock);
      });
}

void FlowZoneAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FlowZoneAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  // Allow up to 16 input channels
  if (layouts.getMainInputChannelSet().size() > 16)
    return false;

  // Accept Stereo or 16-channel Output
  if (layouts.getMainOutputChannelSet().size() != 2 &&
      layouts.getMainOutputChannelSet().size() != 16)
    return false;

  return true;
}
#endif

void FlowZoneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  engine.processBlock(buffer, midiMessages);
}

bool FlowZoneAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *FlowZoneAudioProcessor::createEditor() {
  return new FlowZoneAudioProcessorEditor(*this);
}

void FlowZoneAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {}
void FlowZoneAudioProcessor::setStateInformation(const void *data,
                                                 int sizeInBytes) {}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  auto *p = new FlowZoneAudioProcessor();
  p->init();
  return p;
}
