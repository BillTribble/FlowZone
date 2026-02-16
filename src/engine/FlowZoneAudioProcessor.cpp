#include "FlowZoneAudioProcessor.h"
#include "FlowZoneAudioProcessorEditor.h"

FlowZoneAudioProcessor::FlowZoneAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withOutput("Out 3-4", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 5-6", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 7-8", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 9-10", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 11-12", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 13-14", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out 15-16", juce::AudioChannelSet::stereo(), false)),
      engine(), server(50001)
#endif
{
  // Mark application as active (crash detection)
  crashGuard.markActive();

  // Check for previous crash and safe mode
  if (crashGuard.wasCrashed()) {
    DBG("CrashGuard: Previous crash detected");
    DBG("Safe Mode: " << crashGuard.getSafeModeDescription());
  }

  // Set up WebSocket → CommandQueue flow
  server.setOnMessageCallback([this](const std::string &msg) {
    // Push received command into engine's command queue
    juce::String juceMsg(msg);
    engine.getCommandQueue().push(juceMsg);
  });

  // Set up StateBroadcaster → WebSocket broadcast flow
  engine.getBroadcaster().setMessageCallback(
      [this](const juce::String &msg) { server.broadcast(msg.toStdString()); });

  // Set up initial state callback for new connections
  server.setInitialStateCallback([this]() -> std::string {
    auto state = engine.getSessionManager().getCurrentState();

    // Sync transport state
    state.transport.isPlaying = engine.getTransport().isPlaying();
    state.transport.bpm = engine.getTransport().getBpm();
    state.transport.metronomeEnabled =
        engine.getTransport().isMetronomeEnabled();
    state.transport.loopLengthBars = engine.getTransport().getLoopLengthBars();
    state.transport.barPhase = engine.getTransport().getBarPhase();

    // Create STATE_FULL message
    juce::DynamicObject *root = new juce::DynamicObject();
    root->setProperty("type", "STATE_FULL");
    root->setProperty("revisionId",
                      (juce::int64)engine.getBroadcaster().getRevisionId());
    root->setProperty("data", state.toVar());

    juce::String jsonString = juce::JSON::toString(juce::var(root));
    return jsonString.toStdString();
  });

  server.start();
}

FlowZoneAudioProcessor::~FlowZoneAudioProcessor() {
  // Mark clean shutdown
  crashGuard.markClean();
}

const juce::String FlowZoneAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool FlowZoneAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool FlowZoneAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool FlowZoneAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double FlowZoneAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FlowZoneAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int FlowZoneAudioProcessor::getCurrentProgram() { return 0; }

void FlowZoneAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String FlowZoneAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void FlowZoneAudioProcessor::changeProgramName(int index,
                                               const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void FlowZoneAudioProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock) {
  engine.prepareToPlay(sampleRate, samplesPerBlock);
}

void FlowZoneAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FlowZoneAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // Check Input Layout (Stereo only for now)
  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // Check Output Layout
#if JucePlugin_Build_VST3
  // VST3 Mode: Must be 16 channels (8 stereo pairs) OR Stereo (for simple
  // hosts) We prefer 16 channels for DAW recording. Note: To strictly enforce
  // 16 channels, we would reject others, but strict enforcement can break some
  // hosts. Spec says: "8 stereo pairs (16 channels) for DAW recording via the
  // VST3 plugin target." Let's accept Stereo (2) or 16 channels.
  if (layouts.getMainOutputChannelSet().size() != 2 &&
      layouts.getMainOutputChannelSet().size() != 16)
    return false;
#else
  // Standalone / AU / Other: Stereo only
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
#endif

  return true;
#endif
}
#endif

void FlowZoneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // CRITICAL: Do NOT clear input channels - we need the mic input!
  // Only clear extra output channels that don't have corresponding inputs.
  // The engine needs access to the input audio for mic processing and
  // retrospective looping.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Process with engine - this captures input audio and generates output
  engine.processBlock(buffer, midiMessages);
}

bool FlowZoneAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *FlowZoneAudioProcessor::createEditor() {
  return new FlowZoneAudioProcessorEditor(*this);
}

void FlowZoneAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries
  juce::ignoreUnused(destData);
}

void FlowZoneAudioProcessor::setStateInformation(const void *data,
                                                 int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
  juce::ignoreUnused(data, sizeInBytes);
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FlowZoneAudioProcessor();
}
