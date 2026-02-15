#pragma once
#include "../shared/protocol/schema.h"
#include <JuceHeader.h>

namespace flowzone {

// bd-31s: DiskWriter Tier 1
// Handles writing state to disk
class DiskWriter {
public:
  DiskWriter() {}

  // Tier 1: Simple JSON dump
  static void saveState(const AppState &state, const juce::File &file) {
    juce::var json;
    json.getDynamicObject()->setProperty("bpm", state.bpm);
    json.getDynamicObject()->setProperty("isPlaying", state.isPlaying);

    // Riffs
    juce::Array<juce::var> riffsArray;
    for (const auto &riff : state.activeRiffs) {
      juce::DynamicObject *obj = new juce::DynamicObject();
      obj->setProperty("id", riff.id);
      obj->setProperty("name", riff.name);
      obj->setProperty("lengthBeats", riff.lengthBeats);
      riffsArray.add(juce::var(obj));
    }
    json.getDynamicObject()->setProperty("activeRiffs", riffsArray);

    juce::FileOutputStream stream(file);
    if (stream.openedOk()) {
      stream.setPosition(0);
      stream.truncate();
      juce::JSON::writeToStream(stream, json);
    }
  }

  static AppState loadState(const juce::File &file) {
    AppState state;
    if (!file.existsAsFile())
      return state;

    juce::var json = juce::JSON::parse(file);
    if (json.isObject()) {
      state.bpm = json["bpm"];
      state.isPlaying = json["isPlaying"];

      if (json["activeRiffs"].isArray()) {
        auto riffsArray = *json["activeRiffs"].getArray();
        for (auto &riffVar : riffsArray) {
          Riff riff;
          riff.id = riffVar["id"].toString().toStdString();
          riff.name = riffVar["name"].toString().toStdString();
          riff.lengthBeats = (double)riffVar["lengthBeats"];
          state.activeRiffs.push_back(riff);
        }
      }
    }
    return state;
  }

  // Audio Writing (Tier 1)
  static void writeAudio(const juce::AudioBuffer<float> &buffer,
                         double sampleRate, const juce::File &file) {
    // Ensure directory exists
    file.getParentDirectory().createDirectory();

    if (file.existsAsFile())
      file.deleteFile();

    juce::FlacAudioFormat flacFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(flacFormat.createWriterFor(
        new juce::FileOutputStream(file), sampleRate, buffer.getNumChannels(),
        16, // bits per sample
        {}, // metadata
        0   // compression level (0 = fastest)
        ));

    if (writer) {
      writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }
  }
};

} // namespace flowzone
