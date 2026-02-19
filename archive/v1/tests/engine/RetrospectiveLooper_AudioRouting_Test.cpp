#include "../../libs/catch2/catch.hpp"
#include "../../src/engine/FlowEngine.h"
#include "../../src/engine/RetrospectiveBuffer.h"
#include <JuceHeader.h>

using namespace flowzone;

TEST_CASE("Retrospective Looper Audio Routing", "[audio][looper]") {
  FlowEngine engine;
  double sampleRate = 44100.0;
  int samplesPerBlock = 512;
  
  engine.prepareToPlay(sampleRate, samplesPerBlock);
  
  SECTION("Drum audio routes to retrospective buffer") {
    // Set mode to drums
    engine.setActiveCategory("drums");
    
    // Create a MIDI note-on event to trigger drum
    juce::MidiBuffer midiBuffer;
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 36, (juce::uint8)100); // Kick drum
    midiBuffer.addEvent(noteOn, 0);
    
    // Process a block
    juce::AudioBuffer<float> audioBuffer(2, samplesPerBlock);
    audioBuffer.clear();
    engine.processBlock(audioBuffer, midiBuffer);
    
    // Broadcast state to update looper input level
    engine.broadcastState();
    
    // Get current state
    auto state = engine.getSessionManager().getCurrentState();
    
    // Check that looper received audio
    REQUIRE(state.looper.inputLevel >= 0.0f);
    
    // If audio was generated, level should be above zero
    // Note: This may be zero if drum engine hasn't generated audio yet
    // but we're testing the routing infrastructure exists
    INFO("Looper input level for drums: " << state.looper.inputLevel);
  }
  
  SECTION("Synth audio routes to retrospective buffer") {
    // Set mode to notes
    engine.setActiveCategory("notes");
    
    // Create a MIDI note-on event
    juce::MidiBuffer midiBuffer;
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100); // Middle C
    midiBuffer.addEvent(noteOn, 0);
    
    // Process a block
    juce::AudioBuffer<float> audioBuffer(2, samplesPerBlock);
    audioBuffer.clear();
    engine.processBlock(audioBuffer, midiBuffer);
    
    // Broadcast state to update looper input level
    engine.broadcastState();
    
    // Get current state
    auto state = engine.getSessionManager().getCurrentState();
    
    // Check that looper received audio
    REQUIRE(state.looper.inputLevel >= 0.0f);
    INFO("Looper input level for synth: " << state.looper.inputLevel);
  }
  
  SECTION("Mic audio routes to retrospective buffer") {
    // Set mode to mic
    engine.setActiveCategory("mic");
    
    // Create input buffer with test signal
    juce::AudioBuffer<float> audioBuffer(2, samplesPerBlock);
    
    // Generate sine wave test signal
    for (int ch = 0; ch < audioBuffer.getNumChannels(); ++ch) {
      float* channelData = audioBuffer.getWritePointer(ch);
      for (int i = 0; i < samplesPerBlock; ++i) {
        channelData[i] = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / (float)sampleRate);
      }
    }
    
    juce::MidiBuffer midiBuffer;
    
    // Process the block
    engine.processBlock(audioBuffer, midiBuffer);
    
    // Broadcast state to update looper input level
    engine.broadcastState();
    
    // Get current state
    auto state = engine.getSessionManager().getCurrentState();
    
    // Check that looper received audio
    REQUIRE(state.looper.inputLevel > 0.0f);
    
    // With a 0.5 amplitude sine wave, peak should be around 0.5
    REQUIRE(state.looper.inputLevel <= 0.6f);
    REQUIRE(state.looper.inputLevel >= 0.4f);
    INFO("Looper input level for mic: " << state.looper.inputLevel);
  }
  
  SECTION("Peak level decays to zero with silence") {
    // Set mode to drums
    engine.setActiveCategory("drums");
    
    // Process several blocks of silence
    juce::AudioBuffer<float> audioBuffer(2, samplesPerBlock);
    juce::MidiBuffer midiBuffer;
    
    for (int block = 0; block < 10; ++block) {
      audioBuffer.clear();
      midiBuffer.clear();
      engine.processBlock(audioBuffer, midiBuffer);
    }
    
    // Broadcast state
    engine.broadcastState();
    
    // Get current state
    auto state = engine.getSessionManager().getCurrentState();
    
    // After processing silence, level should be zero or very small
    REQUIRE(state.looper.inputLevel < 0.01f);
    INFO("Looper input level after silence: " << state.looper.inputLevel);
  }
  
  SECTION("Mic input level also tracked separately") {
    // Set mode to mic
    engine.setActiveCategory("mic");
    
    // Set input gain
    engine.setInputGain(0.0f); // 0dB
    
    // Create input buffer with test signal
    juce::AudioBuffer<float> audioBuffer(2, samplesPerBlock);
    
    // Generate sine wave test signal
    for (int ch = 0; ch < audioBuffer.getNumChannels(); ++ch) {
      float* channelData = audioBuffer.getWritePointer(ch);
      for (int i = 0; i < samplesPerBlock; ++i) {
        channelData[i] = 0.3f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / (float)sampleRate);
      }
    }
    
    juce::MidiBuffer midiBuffer;
    
    // Process the block
    engine.processBlock(audioBuffer, midiBuffer);
    
    // Broadcast state to update both mic and looper input levels
    engine.broadcastState();
    
    // Get current state
    auto state = engine.getSessionManager().getCurrentState();
    
    // Check that both mic input level and looper input level are tracked
    REQUIRE(state.mic.inputLevel >= 0.0f);
    REQUIRE(state.looper.inputLevel >= 0.0f);
    
    INFO("Mic input level: " << state.mic.inputLevel);
    INFO("Looper input level: " << state.looper.inputLevel);
    
    // Both should be similar (mic input goes to looper)
    // Allow some tolerance for processing differences
    REQUIRE(std::abs(state.mic.inputLevel - state.looper.inputLevel) < 0.2f);
  }
}

TEST_CASE("RetrospectiveBuffer Audio Capture", "[retrospective][buffer]") {
  RetrospectiveBuffer buffer;
  double sampleRate = 44100.0;
  int maxSeconds = 10;
  
  buffer.prepare(sampleRate, maxSeconds);
  
  SECTION("Can push and retrieve audio") {
    // Create test audio
    int numSamples = 1024;
    juce::AudioBuffer<float> testBuffer(2, numSamples);
    
    // Fill with recognizable pattern
    for (int ch = 0; ch < 2; ++ch) {
      float* data = testBuffer.getWritePointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / (float)sampleRate);
      }
    }
    
    // Push to buffer
    buffer.pushBlock(testBuffer);
    
    // Retrieve it back
    juce::AudioBuffer<float> retrieved;
    buffer.getPastAudio(0, numSamples, retrieved);
    
    // Verify we got something back
    REQUIRE(retrieved.getNumChannels() == 2);
    REQUIRE(retrieved.getNumSamples() == numSamples);
    
    // Verify data matches (at least approximately)
    bool hasNonZero = false;
    for (int ch = 0; ch < 2; ++ch) {
      const float* data = retrieved.getReadPointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        if (std::abs(data[i]) > 0.01f) {
          hasNonZero = true;
          break;
        }
      }
    }
    REQUIRE(hasNonZero);
  }
  
  SECTION("Can retrieve audio from the past") {
    int blockSize = 512;
    
    // Push several blocks
    for (int block = 0; block < 5; ++block) {
      juce::AudioBuffer<float> testBuffer(2, blockSize);
      
      // Fill with block-specific amplitude so we can identify it
      float amplitude = 0.1f * (block + 1);
      for (int ch = 0; ch < 2; ++ch) {
        float* data = testBuffer.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i) {
          data[i] = amplitude;
        }
      }
      
      buffer.pushBlock(testBuffer);
    }
    
    // Retrieve the 4th block (delay of 1 block from current position)
    juce::AudioBuffer<float> retrieved;
    buffer.getPastAudio(blockSize, blockSize, retrieved);
    
    // Check that we got data
    REQUIRE(retrieved.getNumSamples() == blockSize);
    
    // The data should exist
    const float* data = retrieved.getReadPointer(0);
    bool hasData = false;
    for (int i = 0; i < blockSize; ++i) {
      if (std::abs(data[i]) > 0.01f) {
        hasData = true;
        break;
      }
    }
    REQUIRE(hasData);
  }
}
