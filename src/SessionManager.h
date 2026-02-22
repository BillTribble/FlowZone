#pragma once
#include "DiskWriter.h"
#include "Riff.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_events/juce_events.h>

class SessionManager {
public:
  SessionManager() { baseJamFolder = getGlobalJamPointer(); }

  void setJamName(const juce::String &name) {
    baseJamFolder = name;
    juce::File jamDir(getJamPath());
    if (!jamDir.exists())
      jamDir.createDirectory();
    setGlobalJamPointer(baseJamFolder);
  }

  juce::String getJamPath() const {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Samsara")
        .getChildFile(baseJamFolder)
        .getFullPathName();
  }

  void syncRiffToDisk(const Riff &riff) {
    for (int i = 0; i < riff.layers; ++i) {
      if (i >= (int)riff.layerBuffers.size())
        continue;

      juce::String filename =
          "riff_" + riff.id.toString() + "_layer_" + juce::String(i) + ".flac";
      juce::String fullpath =
          juce::File(getJamPath()).getChildFile(filename).getFullPathName();

      // Queue to background thread. The disk writer will overwrite if file
      // exists.
      diskWriter.queueLayerForWriting(riff.layerBuffers[(size_t)i], fullpath,
                                      riff.sourceSampleRate);
    }
  }

  void saveManifest(const RiffHistory &history) {
    juce::var jsonRoot = new juce::DynamicObject();
    auto *rootObj = jsonRoot.getDynamicObject();
    rootObj->setProperty("jam", baseJamFolder);

    juce::Array<juce::var> riffArray;
    for (const auto &r : history.getHistory()) {
      juce::var rVar = new juce::DynamicObject();
      auto *rObj = rVar.getDynamicObject();
      rObj->setProperty("id", r.id.toString());
      rObj->setProperty("name", r.name);
      rObj->setProperty("bpm", r.bpm);
      rObj->setProperty("bars", r.bars);
      rObj->setProperty("layers", r.layers);
      rObj->setProperty("source", r.source);
      rObj->setProperty("sourceSampleRate", r.sourceSampleRate);

      juce::Array<juce::var> gainArray;
      for (auto g : r.layerGains)
        gainArray.add(juce::var((double)g));
      rObj->setProperty("gains", gainArray);

      juce::Array<juce::var> barsArray;
      for (auto b : r.layerBars)
        barsArray.add(juce::var((double)b));
      rObj->setProperty("layerBars", barsArray);

      riffArray.add(rVar);
    }
    rootObj->setProperty("riffs", riffArray);

    juce::File manifest(
        juce::File(getJamPath()).getChildFile("session_state.json"));
    auto outStream = manifest.createOutputStream();
    if (outStream != nullptr) {
      outStream->setPosition(0);
      outStream->truncate();
      juce::JSON::writeToStream(*outStream, jsonRoot);
    }
  }

  void loadFromManifest(RiffHistory &history) {
    juce::File jamDir(getJamPath());
    if (!jamDir.exists())
      jamDir.createDirectory();

    juce::File manifest(jamDir.getChildFile("session_state.json"));
    if (!manifest.existsAsFile())
      return;

    auto parsed = juce::JSON::parse(manifest);
    if (!parsed.isObject())
      return;

    auto *rootObj = parsed.getDynamicObject();
    if (!rootObj->hasProperty("riffs") ||
        !rootObj->getProperty("riffs").isArray())
      return;

    juce::Array<juce::var> *riffArray =
        rootObj->getProperty("riffs").getArray();
    for (auto &riffVar : *riffArray) {
      if (!riffVar.isObject())
        continue;
      auto *rObj = riffVar.getDynamicObject();

      Riff r;
      r.id = juce::Uuid(rObj->getProperty("id").toString());
      r.name = rObj->getProperty("name").toString();
      r.bpm = rObj->getProperty("bpm");
      r.bars = rObj->getProperty("bars");
      int expectedLayers = rObj->getProperty("layers");
      r.source = rObj->getProperty("source").toString();
      if (rObj->hasProperty("sourceSampleRate")) {
        r.sourceSampleRate = rObj->getProperty("sourceSampleRate");
      }

      // Read FLACs synchronously during startup
      for (int i = 0; i < expectedLayers; ++i) {
        juce::File audioFile = jamDir.getChildFile(
            "riff_" + r.id.toString() + "_layer_" + juce::String(i) + ".flac");
        if (audioFile.existsAsFile()) {
          juce::FlacAudioFormat flacFormat;
          std::unique_ptr<juce::AudioFormatReader> reader(
              flacFormat.createReaderFor(new juce::FileInputStream(audioFile),
                                         true));
          if (reader != nullptr) {
            juce::AudioBuffer<float> layerBuffer((int)reader->numChannels,
                                                 (int)reader->lengthInSamples);
            reader->read(&layerBuffer, 0, (int)reader->lengthInSamples, 0, true,
                         true);
            r.layerBuffers.push_back(std::move(layerBuffer));
          }
        }
      }

      if (rObj->hasProperty("gains") && rObj->getProperty("gains").isArray()) {
        juce::Array<juce::var> *gainArray =
            rObj->getProperty("gains").getArray();
        for (auto &gVar : *gainArray)
          r.layerGains.push_back((float)(double)gVar);
      }

      if (rObj->hasProperty("layerBars") &&
          rObj->getProperty("layerBars").isArray()) {
        juce::Array<juce::var> *barsArray =
            rObj->getProperty("layerBars").getArray();
        for (auto &bVar : *barsArray)
          r.layerBars.push_back((float)(double)bVar);
      }

      r.layers = (int)r.layerBuffers.size();
      // Only add riff if we successfully found audio for it
      if (r.layers > 0) {
        history.addRiff(std::move(r));
      }
    }
  }

  static void setGlobalJamPointer(const juce::String &baseJam) {
    juce::File rootDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("Samsara");
    rootDir.createDirectory();
    juce::File pointerFile = rootDir.getChildFile("current_jam.txt");
    pointerFile.replaceWithText(baseJam);
  }

  static juce::String getGlobalJamPointer() {
    juce::File rootDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("Samsara");
    juce::File pointerFile = rootDir.getChildFile("current_jam.txt");
    if (pointerFile.existsAsFile())
      return pointerFile.loadFileAsString().trim();
    return "Gem1";
  }

private:
  juce::String baseJamFolder{"Gem1"};
  DiskWriter diskWriter;
};
