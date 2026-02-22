#include "MainComponent.h"

//==============================================================================
class SamsaraApplication : public juce::JUCEApplication {
public:
  SamsaraApplication() {}

  const juce::String getApplicationName() override { return "Samsara"; }
  const juce::String getApplicationVersion() override { return "2.0.0"; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String & /*commandLine*/) override {
    mainWindow.reset(new MainWindow(getApplicationName()));
  }

  void shutdown() override { mainWindow = nullptr; }

  void systemRequestedQuit() override { quit(); }

  //==========================================================================
  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name)
        : DocumentWindow(name, juce::Colour(0xFF0F0F23),
                         DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);
      setResizable(true, true);
      centreWithSize(getWidth(), getHeight());
      setVisible(true);
    }

    void closeButtonPressed() override {
      JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(SamsaraApplication)
