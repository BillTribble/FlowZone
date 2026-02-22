#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * High-performance FX engine using JUCE DSP library.
 * Now extended to support all 24 V1 Effects via index parameters.
 */
class StandaloneFXEngine {
public:
  StandaloneFXEngine() {
    osc.initialise([](float x) { return std::sin(x); });
    shaper.functionToUse = [](float x) { return std::tanh(x); };
  }

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(
        static_cast<int>(spec.sampleRate * 4.0)); // 4 sec max

    reverb.prepare(spec);
    filter.prepare(spec);
    compressor.prepare(spec);
    phaser.prepare(spec);
    chorus.prepare(spec);
    osc.prepare(spec);
    gate.prepare(spec);
    shaper.prepare(spec);

    reset();
  }

  void setActiveFX(int index) {
    activeFxType = index;
    updateParams();
  }

  void setXY(float x, float y) {
    fxX = juce::jlimit(0.0f, 1.0f, x);
    fxY = juce::jlimit(0.0f, 1.0f, y);
    updateParams();
  }

  void process(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();

    switch (activeFxType) {
    case 0: // Lowpass
    case 1: // Highpass
      filter.process(context);
      break;
    case 2: // Reverb
      reverb.process(context);
      break;
    case 3: // Gate
      gate.process(context);
      break;
    case 4:  // Buzz
    case 14: // Ringmod
    case 17: // Pitchmod (Approximation)
      processRingMod(context);
      break;
    case 5:  // GoTo
    case 12: // Keymasher
    case 19: // Freezer
      processStutter(context);
      break;
    case 6: // Saturator
    case 9: // Distortion
    {
      float driveFactor =
          juce::Decibels::decibelsToGain(juce::jmap(fxX, 0.0f, 40.0f));
      float outGain =
          juce::Decibels::decibelsToGain(juce::jmap(fxY, -20.0f, 0.0f));
      outBlock.multiplyBy(driveFactor);
      shaper.process(context);
      outBlock.multiplyBy(outGain);
    } break;
    case 7:  // Delay
    case 20: // Zap Delay
    case 21: // Dub Delay
      processDelay(context);
      break;
    case 8:  // Comb
    case 18: // Multicomb
      processComb(context);
      break;
    case 10: // Smudge
    case 11: // Channel
      chorus.process(context);
      break;
    case 13: // Ripper
    case 15: // Bitcrush
    case 16: // Degrader
      processBitcrush(context);
      break;
    case 22: // Compressor
      compressor.process(context);
      break;
    case 23: // Trance Gate
      processTranceGate(context);
      break;
    default:
      // No effect
      break;
    }
  }

  void reset() {
    delayLine.reset();
    reverb.reset();
    filter.reset();
    compressor.reset();
    phaser.reset();
    chorus.reset();
    osc.reset();
    gate.reset();
    shaper.reset();
  }

  void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }

private:
  void updateParams() {
    switch (activeFxType) {
    case 0: // Lowpass
      filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
      filter.setCutoffFrequency(juce::jmap(fxX, 20.0f, 20000.0f));
      filter.setResonance(juce::jmap(fxY, 0.1f, 10.0f));
      break;
    case 1: // Highpass
      filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
      filter.setCutoffFrequency(juce::jmap(fxX, 20.0f, 20000.0f));
      filter.setResonance(juce::jmap(fxY, 0.1f, 10.0f));
      break;
    case 2: { // Reverb
      juce::dsp::Reverb::Parameters p;
      p.roomSize = fxX;
      p.wetLevel = fxY;
      p.dryLevel = 1.0f - fxY;
      reverb.setParameters(p);
    } break;
    case 3: // Gate
      gate.setThreshold(juce::jmap(fxX, -60.0f, 0.0f));
      gate.setRelease(juce::jmap(fxY, 10.0f, 1000.0f));
      break;
    case 4:
    case 14: // Ringmod
      osc.setFrequency(juce::jmap(fxX, 20.0f, 2000.0f));
      break;
    case 10:
    case 11: // Chorus
      chorus.setMix(fxX);
      chorus.setDepth(fxY);
      break;
    case 22: // Compressor
      compressor.setThreshold(juce::jmap(fxX, -60.0f, 0.0f));
      compressor.setRatio(juce::jmap(fxY, 1.0f, 20.0f));
      break;
    }
  }

  void processDelay(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float timeSecs = juce::jmap(fxX, 0.01f, 2.0f);
    float feedback = juce::jmap(fxY, 0.0f, 0.95f);

    if (activeFxType == 20) {
      timeSecs = juce::jmap(fxX, 0.001f, 0.1f);
      feedback = juce::jmap(fxY, 0.5f, 1.2f);
    } // Zap Delay
    if (activeFxType == 21) {
      timeSecs = juce::jmap(fxX, 0.1f, 4.0f);
      feedback = fxY;
    } // Dub Delay

    float delayInSamples = timeSecs * (float)sampleRate;
    for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
      auto *samples = outBlock.getChannelPointer(ch);
      for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
        float input = samples[i];
        float delayed = delayLine.popSample((int)ch, delayInSamples);
        float toPush = input + delayed * feedback;

        // Soft clip feedback for stability
        toPush = std::tanh(toPush);

        delayLine.pushSample((int)ch, toPush);
        samples[i] = input + delayed * 0.5f; // Mix
      }
    }
  }

  void processComb(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float freq = juce::jmap(fxX, 50.0f, 5000.0f);
    float delayInSamples = (float)sampleRate / freq;
    float feedback = juce::jmap(fxY, 0.0f, 0.98f);

    for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
      auto *samples = outBlock.getChannelPointer(ch);
      for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
        float input = samples[i];
        float delayed = delayLine.popSample((int)ch, delayInSamples);
        delayLine.pushSample((int)ch, input + delayed * feedback);
        samples[i] = input + delayed;
      }
    }
  }

  void processRingMod(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float depth = fxY;
    for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
      float mod = osc.processSample(0.0f);
      for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
        float input = outBlock.getSample(ch, (int)i);
        outBlock.setSample(ch, (int)i,
                           input * (1.0f - depth) + (input * mod) * depth);
      }
    }
  }

  void processStutter(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float stutterTime = juce::jmap(fxX, 0.01f, 0.5f);
    float delayInSamples = stutterTime * (float)sampleRate;
    float freezeMix = fxY > 0.5f ? 1.0f : 0.0f;

    for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
      auto *samples = outBlock.getChannelPointer(ch);
      for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
        float input = samples[i];
        float delayed = delayLine.popSample((int)ch, delayInSamples);

        if (freezeMix > 0.5f) {
          delayLine.pushSample((int)ch, delayed);
          samples[i] = delayed;
        } else {
          delayLine.pushSample((int)ch, input);
          samples[i] = input;
        }
      }
    }
  }

  void processBitcrush(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float bits = juce::jmap(fxX, 1.0f, 16.0f);
    float levels = std::pow(2.0f, bits);
    int skipSteps = static_cast<int>(juce::jmap(fxY, 1.0f, 32.0f));

    for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
      auto *samples = outBlock.getChannelPointer(ch);
      float heldSample = 0.0f;
      for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
        if (i % skipSteps == 0) {
          float input = samples[i];
          heldSample = std::round(input * levels) / levels;
        }
        samples[i] = heldSample;
      }
    }
  }

  void processTranceGate(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float depth = fxY;
    float rate = juce::jmap(fxX, 1.0f, 16.0f);
    double samplesPerBeat = sampleRate / rate;

    for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
      trancePhase += 1.0;
      if (trancePhase > samplesPerBeat)
        trancePhase -= samplesPerBeat;

      float gateVal = (trancePhase < samplesPerBeat * 0.5) ? 1.0f : 0.0f;
      gateVal = 1.0f - (depth * (1.0f - gateVal));

      for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
        outBlock.setSample(ch, (int)i,
                           outBlock.getSample(ch, (int)i) * gateVal);
      }
    }
  }

  // --- Processors ---
  juce::dsp::DelayLine<float> delayLine;
  juce::dsp::Reverb reverb;
  juce::dsp::StateVariableTPTFilter<float> filter;
  juce::dsp::Compressor<float> compressor;
  juce::dsp::Phaser<float> phaser;
  juce::dsp::Chorus<float> chorus;
  juce::dsp::Oscillator<float> osc;
  juce::dsp::NoiseGate<float> gate;
  juce::dsp::WaveShaper<float> shaper;

  // --- State ---
  int activeFxType = 0;
  float fxX = 0.5f;
  float fxY = 0.5f;
  double sampleRate = 44100.0;
  double trancePhase = 0.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneFXEngine)
};
