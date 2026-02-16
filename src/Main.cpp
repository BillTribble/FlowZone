/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "engine/FlowEngine.h"
#include "engine/server/WebSocketServer.h"
#include <JuceHeader.h>

//==============================================================================
class FlowZoneStandaloneApplication : public juce::JUCEApplication,
                                      public juce::AudioIODeviceCallback {
public:
  //==============================================================================
  FlowZoneStandaloneApplication() {}

  const juce::String getApplicationName() override {
    return ProjectInfo::projectName;
  }
  const juce::String getApplicationVersion() override {
    return ProjectInfo::versionString;
  }
  bool moreThanOneInstanceAllowed() override { return true; }

  //==============================================================================
  //==============================================================================
  void initialise(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);

    // 1. Initialize Engine
    engine.reset(new flowzone::FlowEngine());

    // 2. Initialize Audio Device Manager
    audioDeviceManager.reset(new juce::AudioDeviceManager());

    // Initialize audio with stereo input and output
    juce::String error =
        audioDeviceManager->initialise(2,       // number of input channels
                                       2,       // number of output channels
                                       nullptr, // no saved state
                                       true     // select default device
        );

    if (error.isNotEmpty()) {
      juce::Logger::writeToLog("Audio Device Error: " + error);
    } else {
      juce::Logger::writeToLog("Audio Device initialized successfully");
      juce::Logger::writeToLog(
          "Input device: " +
          (audioDeviceManager->getCurrentAudioDevice()
               ? audioDeviceManager->getCurrentAudioDevice()->getName()
               : "None"));
    }

    // Set up audio callback
    audioDeviceManager->addAudioCallback(this);

    // Prepare engine for audio processing
    if (auto *device = audioDeviceManager->getCurrentAudioDevice()) {
      engine->prepareToPlay(device->getCurrentSampleRate(),
                            device->getCurrentBufferSizeSamples());
    }

    // 3. Initialize Server
    server.reset(new WebSocketServer(50001));

    // 4. Connect Broadcaster to Server
    engine->getBroadcaster().setMessageCallback(
        [this](const juce::String &msg) {
          if (server) {
            server->broadcast(msg.toStdString());
          }
        });

    // 5. Setup Initial State Callback
    server->setInitialStateCallback([this]() -> std::string {
      if (engine) {
        auto state = engine->getSessionManager().getCurrentState();
        juce::DynamicObject *root = new juce::DynamicObject();
        root->setProperty("type", "STATE_FULL");
        root->setProperty("revisionId",
                          engine->getBroadcaster().getRevisionId());
        root->setProperty("data", state.toVar());
        return juce::JSON::toString(juce::var(root)).toStdString();
      }
      return "{}";
    });

    // 6. Setup Message Handling (Commands from Frontend)
    server->setOnMessageCallback([this](const std::string &msg) {
      if (engine) {
        engine->getCommandQueue().push(juce::String(msg));
      }
    });

    server->start();

    // 7. Create and show the main window
    mainWindow.reset(new MainWindow(getApplicationName()));

    juce::Logger::writeToLog("FlowZone Engine Started on port 50001");
  }

  void shutdown() override {
    // Clean shutdown: remove audio callback before destroying engine
    if (audioDeviceManager) {
      audioDeviceManager->removeAudioCallback(this);
      audioDeviceManager->closeAudioDevice();
    }

    server->stop();
    server.reset();
    engine.reset();
    audioDeviceManager.reset();
    mainWindow = nullptr; // (deletes our window)
  }

  //==============================================================================
  void systemRequestedQuit() override {
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app
    // to close.
    quit();
  }

  //==============================================================================
  // AudioIODeviceCallback implementation
  void audioDeviceIOCallbackWithContext(
      const float *const *inputChannelData, int numInputChannels,
      float *const *outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext &context) override {
    juce::ignoreUnused(context);

    if (!engine)
      return;

    // Create buffers for processing
    juce::AudioBuffer<float> buffer(numOutputChannels, numSamples);
    juce::MidiBuffer midiMessages;

    // Copy input to buffer
    for (int ch = 0; ch < numInputChannels && ch < numOutputChannels; ++ch) {
      if (inputChannelData[ch] != nullptr) {
        buffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
      }
    }

    // Process through engine
    engine->processBlock(buffer, midiMessages);

    // Copy output from buffer
    for (int ch = 0; ch < numOutputChannels; ++ch) {
      if (outputChannelData[ch] != nullptr) {
        memcpy(outputChannelData[ch], buffer.getReadPointer(ch),
               sizeof(float) * numSamples);
      }
    }
  }

  void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
    if (engine && device) {
      engine->prepareToPlay(device->getCurrentSampleRate(),
                            device->getCurrentBufferSizeSamples());
    }
  }

  void audioDeviceStopped() override {
    // Engine cleanup if needed
  }

  void anotherInstanceStarted(const juce::String &commandLine) override {
    // When another instance of the app is launched while this one is running,
    // this method is invoked, and the commandLine parameter tells you what
    // the other instance's command-line arguments were.
    juce::ignoreUnused(commandLine);
  }

  //==============================================================================
  /*
      This class implements the desktop window that contains an instance of
      our MainComponent class.
  */
  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name)
        : DocumentWindow(
              name,
              juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                  juce::ResizableWindow::backgroundColourId),
              DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      // setContentOwned (new MainComponent(), true);

      // For now, just a placeholder window since UI is web-based
      juce::Label *label = new juce::Label(
          "info", "FlowZone Backend Running.\nConnect via Web Client.");
      label->setJustificationType(juce::Justification::centred);
      setContentOwned(label, true);

#if JUCE_IOS || JUCE_ANDROID
      setFullScreen(true);
#else
      setResizable(true, true);
      centreWithSize(300, 200);
#endif

      setVisible(true);
    }

    void closeButtonPressed() override {
      // This is called when the user tries to close this window. Here, we'll
      // just ask the app to quit when this happens, but you can change this to
      // do whatever you need.
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    /* Note: Be careful if you override any DocumentWindow methods - the base
       class uses a lot of them, so by overriding you might break its
       functionality. It's best to do all your work in your content component
       instead, but if you really have to override any DocumentWindow methods,
       make sure your subclass also calls the superclass's method.
    */

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  std::unique_ptr<flowzone::FlowEngine> engine;
  std::unique_ptr<WebSocketServer> server;
  std::unique_ptr<juce::AudioDeviceManager> audioDeviceManager;
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(FlowZoneStandaloneApplication)
