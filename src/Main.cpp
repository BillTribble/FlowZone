/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "engine/FlowEngine.h"
#include "engine/server/WebSocketServer.h"
#include <JuceHeader.h>

//==============================================================================
class FlowZoneStandaloneApplication : public juce::JUCEApplication {
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

    // 2. Initialize Server
    server.reset(new WebSocketServer(50001));

    // 3. Connect Broadcaster to Server
    engine->getBroadcaster().setMessageCallback(
        [this](const juce::String &msg) {
          if (server) {
            server->broadcast(msg.toStdString());
          }
        });

    // 4. Setup Initial State Callback
    server->setInitialStateCallback([this]() -> std::string {
      if (engine) {
        auto state = engine->getSessionManager().getCurrentState();
        return juce::JSON::toString(state.toVar()).toStdString();
      }
      return "{}";
    });

    // 5. Setup Message Handling (Commands from Frontend)
    server->setOnMessageCallback([this](const std::string &msg) {
      if (engine) {
        engine->getCommandQueue().push(juce::String(msg));
      }
    });

    server->start();

    juce::Logger::writeToLog("FlowZone Engine Started on port 50001");
  }

  void shutdown() override {
    server->stop();
    server.reset();
    engine.reset();
    mainWindow = nullptr; // (deletes our window)
  }

  //==============================================================================
  void systemRequestedQuit() override {
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app
    // to close.
    quit();
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
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(FlowZoneStandaloneApplication)
