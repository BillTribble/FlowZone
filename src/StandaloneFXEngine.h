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
      processStutterGate(context, false);
      break;
    case 5: // Gate Trip
      processStutterGate(context, true);
      break;
    case 4:  // Buzz
    case 13: // Ringmod
    case 16: // Pitchmod (Approximation)
      processRingMod(context);
      break;
    case 12: // Keymasher
    case 18: // Freezer
      processStutter(context);
      break;
    case 6: // Distort
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
    case 19: // Zap Delay
    case 20: // Dub Delay
      processDelay(context);
      break;
    case 8:  // Comb
    case 17: // Multicomb
      processComb(context);
      break;
    case 9: // Smudge
      chorus.process(context);
      break;
    case 14: // Bitcrush
    case 15: // Degrader
      processBitcrush(context);
      break;
    case 21: // Trance Gate
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
      filter.setCutoffFrequency(20.0f * std::pow(1000.0f, fxX));
      filter.setResonance(juce::jmap(fxY, 0.1f, 10.0f));
      break;
    case 1: // Highpass
      filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
      filter.setCutoffFrequency(20.0f * std::pow(1000.0f, fxX));
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
      // No standard JUCE params to update for Gate FX, it's hand-rolled
      break;
    case 4:
    case 13: // Ringmod
      osc.setFrequency(juce::jmap(fxX, 20.0f, 2000.0f));
      break;
    case 9: // Smudge
      chorus.setMix(fxX);
      chorus.setDepth(fxY);
      break;
    }
  }

  void processDelay(juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outBlock = context.getOutputBlock();
    float timeSecs = juce::jmap(fxX, 0.01f, 2.0f);
    float feedback = juce::jmap(fxY, 0.0f, 0.95f);

    if (activeFxType == 19) {
      timeSecs = juce::jmap(fxX, 0.001f, 0.1f);
      feedback = juce::jmap(fxY, 0.5f, 1.2f);
    } // Zap Delay
    if (activeFxType == 20) {
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

    // 16-step patterns. 1 is audio pass, 0 is audio cut.
    uint16_t patterns[16] = {
        0xFFFF, // 0: All on
        0xAAAA, // 1: 1010101010101010 (every other)
        0xCCCC, // 2: 1100110011001100 (pairs)
        0xF0F0, // 3: 1111000011110000 (fours)
        0xEEEE, // 4: 1110111011101110 (off beat cut)
        0x8888, // 5: 1000100010001000 (on beats)
        0x2222, // 6: 0010001000100010 (off beats)
        0xBA98, // 7: 1011101010011000 (syncopated 1)
        0xE38E, // 8: 1110001110001110 (poly 3 over 4)
        0xFE7F, // 9: 1111111001111111 (snare skip)
        0xA5A5, // 10: 1010010110100101
        0xFF00, // 11: 1111111100000000 (half bar)
        0xC3C3, // 12: 1100001111000011
        0x9249, // 13: 1001001001001001 (triplet feel)
        0x5555, // 14: 0101010101010101
        0x1111  // 15: 0001000100010001
    };

    int patternIdx = juce::jlimit(0, 15, static_cast<int>(fxX * 16.0f));
    if (patternIdx == 16)
      patternIdx = 15;

    uint16_t currentPattern = patterns[patternIdx];
    float depth = fxY;

    double samplesPerBeat = sampleRate / (currentBpm / 60.0);
    double samplesPerStep = samplesPerBeat / 4.0;

    for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
      trancePhase += 1.0;
      if (trancePhase >= samplesPerStep * 16.0)
        trancePhase -= samplesPerStep * 16.0;

      int currentStep = static_cast<int>(trancePhase / samplesPerStep);
      currentStep = juce::jlimit(0, 15, currentStep);

      bool bitOn = (currentPattern & (1 << (15 - currentStep))) != 0;

      float targetGate = bitOn ? 1.0f : 0.0f;
      targetGate = 1.0f - (depth * (1.0f - targetGate));

      gateSmooth = 0.95f * gateSmooth + 0.05f * targetGate;

      for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
        outBlock.setSample(ch, (int)i,
                           outBlock.getSample(ch, (int)i) * gateSmooth);
      }
    }
  }

  void processStutterGate(juce::dsp::ProcessContextReplacing<float> &context,
                          bool isTriplet) {
    auto &outBlock = context.getOutputBlock();

    double divisionsNormal[] = {4.0, 8.0, 16.0, 32.0};
    double divisionsTriplet[] = {6.0, 12.0, 24.0,
                                 48.0}; // 1/4T, 1/8T, 1/16T, 1/32T

    int step = juce::jlimit(0, 3, static_cast<int>(fxX * 4.0f));
    if (step == 4)
      step = 3;

    double rate = isTriplet ? divisionsTriplet[step] : divisionsNormal[step];
    double samplesPerBeat = sampleRate / (currentBpm / 60.0);
    double samplesPerGate = samplesPerBeat * (4.0 / rate);

    float dutyCycle = juce::jmap(fxY, 0.9f, 0.1f);

    for (size_t i = 0; i < outBlock.getNumSamples(); ++i) {
      stutterPhase += 1.0;
      if (stutterPhase >= samplesPerGate)
        stutterPhase -= samplesPerGate;

      float gateVal = (stutterPhase < samplesPerGate * dutyCycle) ? 1.0f : 0.0f;
      gateSmooth =
          0.9f * gateSmooth + 0.1f * gateVal; // smoothing to prevent clicking

      for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch) {
        outBlock.setSample(ch, (int)i,
                           outBlock.getSample(ch, (int)i) * gateSmooth);
      }
    }
  }

  // --- Processors ---
  juce::dsp::DelayLine<float> delayLine;
  juce::dsp::Reverb reverb;
  juce::dsp::StateVariableTPTFilter<float> filter;
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
  double currentBpm = 120.0;
  double trancePhase = 0.0;
  double stutterPhase = 0.0;
  float gateSmooth = 0.0f;

public:
  void setBpm(double newBpm) { currentBpm = newBpm; }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneFXEngine)
};
