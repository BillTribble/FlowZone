# FlowZone Audio Engine Specifications

**Appendix to Spec 1.6** — Detailed specifications for all internal audio engines, effects, and sound generators.

---

## **1. InternalFX — Effects Processing**

\u003e **Implementation Note:** These effects support all UI elements defined in §7.6.3 (Sound/FX Tab) of the main spec. Simple procedural implementations for V1; future versions may replace with higher-quality algorithms or allow VST alternatives.

### **1.1. Core FX Bank (12 Effects)**

1.  **Lowpass** — 2-pole Butterworth filter
    *   Parameters: Cutoff (20Hz - 20kHz), Resonance (0.1 - 10.0)
    *   XY Mapping: X = Cutoff, Y = Resonance

2.  **Highpass** — 2-pole Butterworth filter
    *   Parameters: Cutoff (20Hz - 20kHz), Resonance (0.1 - 10.0)
    *   XY Mapping: X = Cutoff, Y = Resonance

3.  **Reverb** — Freeverb algorithm
    *   Parameters: Room Size (0.0 - 1.0), Damping (0.0 - 1.0), Wet/Dry (0.0 - 1.0)
    *   XY Mapping: X = Room Size, Y = Wet/Dry

4.  **Gate** — Amplitude gate with envelope follower
    *   Parameters: Threshold (-60dB - 0dB), Attack (1ms - 100ms), Release (10ms - 1000ms)
    *   XY Mapping: X = Threshold, Y = Release

5.  **Buzz** — Ring modulator at fixed frequency
    *   Parameters: Frequency (20Hz - 2kHz), Depth (0.0 - 1.0)
    *   XY Mapping: X = Frequency, Y = Depth

6.  **GoTo** — Playhead jump effect
    *   Parameters: Jump Probability (0.0 - 1.0), Quantize Grid (1/16, 1/8, 1/4, 1/2)
    *   XY Mapping: X = Probability, Y = Grid Size

7.  **Saturator** — Soft-clip waveshaper
    *   Parameters: Drive (0dB - 40dB), Output Gain (-20dB - 0dB)
    *   XY Mapping: X = Drive, Y = Output Gain

8.  **Delay** — Tempo-synced delay line
    *   Parameters: Time (1/16 - 4 bars), Feedback (0.0 - 0.95), Wet/Dry (0.0 - 1.0)
    *   XY Mapping: X = Time, Y = Feedback

9.  **Comb** — Comb filter for metallic tones
    *   Parameters: Frequency (50Hz - 5kHz), Feedback (0.0 - 0.98)
    *   XY Mapping: X = Frequency, Y = Feedback

10. **Distortion** — Hard-clip waveshaper with tone control
    *   Parameters: Drive (0dB - 60dB), Tone (500Hz - 10kHz lowpass), Output Gain (-20dB - 0dB)
    *   XY Mapping: X = Drive, Y = Tone

11. **Smudge** — Pitch-shifting granular effect
    *   Parameters: Pitch Shift (-24 to +24 semitones), Grain Size (10ms - 200ms)
    *   XY Mapping: X = Pitch Shift, Y = Grain Size

12. **Channel** — Stereo width and panning
    *   Parameters: Width (0.0 - 2.0, where 1.0 = normal stereo), Pan (-1.0 to +1.0)
    *   XY Mapping: X = Width, Y = Pan

### **1.2. Infinite FX Bank (11 Effects)**

1.  **Keymasher** — 12-button performance sampler (§7.6.3 button grid)
    *   No XY Pad — uses 3×4 button grid instead
    *   Buttons: Repeat, Pitch Down, Pitch Rst, Pitch Up, Reverse, Gate, Scratch, Buzz, Stutter, Go To, Go To 2, Buzz Slip
    *   Implementation: Captures loop buffer, applies real-time manipulation per button

2.  **Ripper** — Bit reduction + sample rate reduction
    *   Parameters: Bit Depth (1 - 16 bits), Sample Rate Reduction (1x - 32x)
    *   XY Mapping: X = Bit Depth, Y = Sample Rate Reduction

3.  **Ringmod** — Ring modulator with LFO
    *   Parameters: Carrier Frequency (20Hz - 5kHz), LFO Rate (0.1Hz - 20Hz), Depth (0.0 - 1.0)
    *   XY Mapping: X = Carrier Frequency, Y = LFO Rate

4.  **Bitcrusher** — Bit reduction only
    *   Parameters: Bit Depth (1 - 16 bits), Dither Amount (0.0 - 1.0)
    *   XY Mapping: X = Bit Depth, Y = Dither

5.  **Degrader** — Combined lo-fi effect (bit reduction + noise + filtering)
    *   Parameters: Degradation Amount (0.0 - 1.0), Noise Level (0.0 - 0.5), Filter Cutoff (500Hz - 10kHz)
    *   XY Mapping: X = Degradation, Y = Noise Level

6.  **Pitchmod** — Pitch shifter with modulation
    *   Parameters: Semitones (-12 to +12), LFO Rate (0.1Hz - 10Hz), Depth (0.0 - 12 semitones)
    *   XY Mapping: X = Semitones, Y = LFO Depth

7.  **Multicomb** — Parallel comb filters
    *   Parameters: Base Frequency (50Hz - 2kHz), Frequency Spread (0.0 - 1.0), Feedback (0.0 - 0.98)
    *   XY Mapping: X = Base Frequency, Y = Spread

8.  **Freezer** — Buffer freeze/hold effect
    *   Parameters: Freeze (toggle), Fade Time (10ms - 1000ms)
    *   XY Mapping: X = Crossfade Position (frozen ← → live), Y = Fade Time

9.  **Zap Delay** — Short delay with high feedback
    *   Parameters: Time (1ms - 100ms), Feedback (0.0 - 1.2), Filter Cutoff (200Hz - 10kHz)
    *   XY Mapping: X = Time, Y = Feedback

10. **Dub Delay** — Long delay with filtering (tempo-synced)
    *   Parameters: Time (1/8 - 8 bars), Feedback (0.0 - 0.99), Lowpass Cutoff (500Hz - 10kHz)
    *   XY Mapping: X = Time, Y = Feedback

11. **Compressor** — Dynamics processor
    *   Parameters: Threshold (-40dB - 0dB), Ratio (1:1 - 20:1), Attack (0.1ms - 100ms), Release (10ms - 1000ms)
    *   XY Mapping: X = Threshold, Y = Ratio

---

## **2. InternalSynth — Notes Mode Presets**

\u003e **Implementation Note:** Simple procedural synths using basic waveforms + dual ADSR envelopes + filter. Designed to be functional, not production-quality. Future versions will replace with sample-based instruments or external VSTs.

### **2.0. Synthesis Architecture**

All InternalSynth presets (Notes and Bass modes) use the following signal chain:

```
Oscillator(s) → Amplitude Envelope → Filter → Filter Envelope → Output
```

**Dual-Envelope Design:**
1. **Amplitude Envelope (ADSR)** — Controls volume over time
   - Attack, Decay, Sustain, Release
   - Applied to oscillator output before filtering
2. **Filter Envelope (ADSR)** — Controls filter cutoff over time
   - Separate Attack, Decay, Sustain, Release
   - Modulates filter cutoff frequency
   - Amount can be positive (opens filter) or negative (closes filter)

**Parameter Exposure:**
- Both envelopes are independent and fully controllable
- UI knobs (Bounce, Length) affect amplitude envelope by default
- Filter envelope can be adjusted via preset-specific parameters
- Some presets use only amplitude envelope (e.g., Sine Bell has no filter)

### **2.1. 12 Starting Presets**

1.  **Sine Bell** — Pure sine wave, medium decay (0.8s), no filter

2.  **Saw Lead** — Sawtooth wave, short decay (0.3s), lowpass filter (cutoff: 3kHz, res: 0.5)

3.  **Square Bass** — Square wave (-1 octave), long sustain (0.7), gentle lowpass (cutoff: 1kHz)

4.  **Triangle Pad** — Triangle wave, long attack (0.5s) / release (1.5s), subtle vibrato (LFO: 4Hz, depth: 5 cents)

5.  **Pluck** — Sawtooth, very short decay (0.05s), simulates plucked string

6.  **Warm Pad** — 50% Saw + 50% Triangle blend, long attack (0.8s), chorus effect (3 voices detuned)

7.  **Bright Lead** — Sawtooth, short envelope (A:0.01s D:0.2s S:0.0), filter envelope (cutoff: 500Hz → 8kHz, resonance: 2.0)

8.  **Soft Keys** — 80% Sine + 20% Triangle, medium decay (0.6s), 3 detuned voices (±10 cents)

9.  **Organ** — Additive synthesis (sine harmonics 1.0, 0.5, 0.25, 0.125), no envelope, slight vibrato (LFO: 6Hz, depth: 3 cents)

10. **EP** — Sine wave with FM modulation (mod ratio: 1:1.5, index: 2.0), short-medium decay (0.4s)

11. **Choir** — 4 detuned sawtooth oscillators (±20 cents), slow attack (1.0s), gentle lowpass (cutoff: 4kHz, resonance: 0.3)

12. **Arp** — Sawtooth, gate pattern via LFO (rate: tempo-synced 1/16), filter envelope (cutoff sweep: 300Hz → 5kHz)

### **2.2. Common Parameter Mapping**

Maps to Adjust Tab knobs (§7.6.4 in main spec):

*   **Pitch** → Transpose (-24 to +24 semitones)
*   **Length** → Decay/Release Time Multiplier (0.1x - 5.0x)
*   **Tone** → Filter Cutoff (100Hz - 10kHz, or filter amount if no filter)
*   **Level** → Output Gain (-60dB - +6dB)
*   **Bounce** → Envelope Attack Time (0.001s - 2.0s)
*   **Speed** → LFO Rate (0.1Hz - 20Hz, where applicable)
*   **Reverb** → Reverb Send Level (0.0 - 1.0)

### **2.3. Microtuning Support**

*   All presets support `.scl` (Scala scale) and `.kbm` (keyboard mapping) files
*   Default: 12-tone equal temperament (12TET)
*   Preset tunings included: Just Intonation, Pythagorean, Slendro, Pelog (as specified in §1.1 of main spec)

---

## **3. InternalSynth — Bass Mode Presets**

Same synthesis architecture as Notes Mode (§2.0 — dual-envelope design), optimized for low frequencies.

### **3.1. 12 Starting Presets**

1.  **Sub** — Pure sine wave, 1 octave down, short envelope (A:0.01s D:0.3s S:0.0 R:0.1s), no filter

2.  **Growl** — 50% Saw + 50% Square, filter envelope (500Hz → 3kHz), high resonance (3.0)

3.  **Deep** — Triangle wave, long sustain (0.8), gentle lowpass (cutoff: 800Hz, resonance: 0.2)

4.  **Wobble** — Sawtooth, LFO-modulated filter (rate: tempo-synced 1/4 note), high resonance (4.0)

5.  **Punch** — Square wave, very short attack (0.001s), fast filter close (envelope: 2kHz → 300Hz over 0.1s)

6.  **808** — Sine wave with pitch envelope (120Hz → 40Hz over 0.15s), mimics TR-808 kick but sustained

7.  **Fuzz** — Sawtooth + distortion (drive: 30dB), aggressive lowpass filter (cutoff: 1.5kHz, resonance: 1.5)

8.  **Reese** — 3 detuned sawtooth oscillators (±15 cents), subtle chorus, gentle lowpass (cutoff: 2kHz)

9.  **Smooth** — 70% Sine + 30% Sawtooth blend, medium decay (0.5s), no filter

10. **Rumble** — Sine wave (very low: -2 octaves), slow attack (0.3s), long release (2.0s)

11. **Pluck Bass** — Sawtooth, very short decay (0.08s), comb filter tuned to fundamental

12. **Acid** — Sawtooth, filter envelope with accent sensitivity (cutoff: 200Hz → 4kHz), resonance: 5.0 (TB-303 style)

### **3.2. Parameter Mapping**

Same as Notes Mode (§2.2).

---

## **4. DrumEngine — Procedural Drum Synthesis**

\u003e **Implementation Note:** Simple procedural synthesis for 16 drum sounds. Maps to the 4×4 pad grid (§7.6.2 in main spec). Icon-to-sound mapping based on mobile layout reference.

### **4.1. Pad Layout \u0026 Sound Mapping**

| Pad (Row, Col) | Icon | Sound Type | Synthesis Method |
|:---|:---|:---|:---|
| (1,1) | `double_diamond` | Kick 1 | Sine wave, pitch envelope (80Hz → 30Hz), short decay (0.3s) |
| (1,2) | `cylinder` | Tom Low | Sine wave, pitch envelope (120Hz → 80Hz), medium decay (0.5s) |
| (1,3) | `tall_cylinder` | Tom Mid | Sine wave, pitch envelope (200Hz → 150Hz), medium decay (0.4s) |
| (1,4) | `tripod` | Cymbal | Metallic noise (bandpass: 3kHz - 12kHz), long decay (2.0s) |
| (2,1) | `double_diamond` | Kick 2 | Sine + triangle blend, pitch envelope (100Hz → 40Hz), punchier attack |
| (2,2) | `double_diamond` | Kick 3 | Sine wave, pitch envelope (60Hz → 25Hz), longer decay (0.5s) |
| (2,3) | `double_diamond` | Kick 4 | Square wave, pitch envelope (90Hz → 35Hz), distorted |
| (2,4) | `double_diamond` | Tom High | Sine wave, pitch envelope (300Hz → 220Hz), short decay (0.3s) |
| (3,1) | `hand` | Clap | Filtered noise burst (bandpass: 800Hz - 3kHz), multiple hits (3×), medium decay (0.2s) |
| (3,2) | `hand` | Snap | Filtered noise burst (bandpass: 2kHz - 8kHz), single hit, very short (0.05s) |
| (3,3) | `snare` | Snare 1 | Filtered noise (1kHz - 5kHz) + sine tone (200Hz), snappy envelope (0.15s) |
| (3,4) | `snare` | Snare 2 | Filtered noise (800Hz - 4kHz) + triangle tone (180Hz), longer tail (0.25s) |
| (4,1) | `lollipop` | Hi-Hat Closed | High-freq noise (5kHz - 15kHz), very short decay (0.05s) |
| (4,2) | `lollipop` | Hi-Hat Open | High-freq noise (4kHz - 12kHz), medium decay (0.3s) |
| (4,3) | `lollipop` | Hi-Hat Pedal | High-freq noise (3kHz - 10kHz), short decay (0.1s) |
| (4,4) | `lollipop` | Ride | Metallic noise (2kHz - 8kHz), long sustain (1.0s) |

### **4.2. 4 Drum Kit Presets**

As listed in mobile UI §7.6.3:

1.  **Synthetic** — Clean procedural sounds (table above shows Synthetic kit sounds)
2.  **Lo-Fi** — Same synthesis but with bit reduction (8-bit) + sample rate reduction (22kHz)
3.  **Acoustic** — Enhanced pitch envelopes + subtle reverb to mimic acoustic drums
4.  **Electronic** — Sharper attacks, distortion on kicks, filtered noise on snares

### **4.3. Parameter Mapping**

Adjust Tab controls:

*   **Pitch** → Tune individual pad (-12 to +12 semitones)
*   **Tone** → Filter cutoff (brightens/darkens the sound)
*   **Level** → Output gain per pad

---

## **5. SampleEngine — Sample Playback System**

\u003e **Future-Proof Architecture:** Designed to replace procedural sounds with high-quality samples. V1 implementation supports basic playback; V2+ will add advanced features (multi-sampling, velocity layers, round-robin).

### **5.1. Sample Loading**

*   **Supported formats:** WAV, FLAC, MP3, AIFF (via JUCE `AudioFormatManager`)
*   **Maximum sample length:** 60 seconds (longer samples truncated with warning)
*   **Sample rate conversion:** Auto-resample to session sample rate (44.1kHz or 48kHz)

### **5.2. Playback Modes**

1.  **One-Shot:** Trigger plays sample start-to-end, ignores note-off
2.  **Gated:** Plays while pad held, stops on release (with fade-out to avoid clicks)
3.  **Looped:** Plays sample in loop (uses loop points if defined in file metadata, otherwise loops full sample)
4.  **Sliced:** Divides sample into 16 equal slices, maps one slice per pad (4×4 grid)

### **5.3. Parameters**

*   **Pitch:** Transpose playback (-24 to +24 semitones), implemented via resampling
*   **Start Offset:** Where in the sample playback begins (0.0 - 1.0)
*   **Loop Points:** Define loop start/end for Looped mode (0.0 - 1.0 for each)
*   **ADSR Envelope:** Applied to sample output (Attack, Decay, Sustain, Release)
*   **Filter:** Optional lowpass/highpass filter (cutoff: 20Hz - 20kHz, resonance: 0.1 - 10.0)
*   **Reverse:** Play sample backwards (toggle)

### **5.4. Preset System**

Presets are JSON files: `/assets/presets/sampler/[preset_name].json`

Each preset defines:
*   Sample file paths (can be multiple samples with note/velocity mapping)
*   Pad assignments (which sample plays on which pad)
*   Default parameters (pitch, envelope, filter settings)

**Example JSON structure:**
```json
{
  "id": "acoustic_drums",
  "name": "Acoustic Drums",
  "category": "sampler",
  "type": "sample",
  "mode": "one_shot",
  "pads": [
    { "index": 0, "sample": "samples/drums/kick.wav", "pitch": 0 },
    { "index": 1, "sample": "samples/drums/snare.wav", "pitch": 0 }
  ],
  "envelope": { "attack": 0.001, "decay": 0.0, "sustain": 1.0, "release": 0.01 }
}
```

### **5.5. Drag-and-Drop / File Picker**

*   **Desktop:** User drags audio file from Finder onto a pad → sample auto-loads to that pad
*   **Mobile:** Tap "Add" button → file picker opens → select audio file → assign to pad
*   Files are copied to project directory: `~/Library/Application Support/FlowZone/[session_id]/samples/`

### **5.6. Migration Path from Procedural Sounds**

*   All procedural drum/synth presets can export their output as samples (bounce to audio)
*   UI command: Long-press preset → "Export as Sample" → saves to sampler library
*   This allows users to "freeze" procedural sounds for consistency across sessions

### **5.7. Performance Considerations**

*   Samples are loaded into RAM (pre-buffered) to avoid disk I/O during playback
*   Maximum RAM allocation for samples: 500MB per session
*   If limit exceeded, show warning: "Sample library exceeds 500MB. Remove unused samples."

---

## **6. MicProcessor — Input Processing Chain**

Handles microphone/line input with real-time effects.

### **6.1. Input Monitoring**

*   **Monitor Until Looped** (toggle) — Input is audible until first loop commit, then muted
*   **Monitor Input** (toggle) — Always monitor input (can cause feedback if using built-in mic + speakers)

### **6.2. Input Gain**

*   Adjustable via large circular knob (§7.6.6 in main spec)
*   Range: -60dB to +40dB
*   Visual: Arc indicator shows current gain level
*   Auto-gain option (future): Automatic level adjustment

### **6.3. Input FX Chain**

*   Same FX as InternalFX (§1) can be applied to input
*   Common use: Lowpass/Highpass filtering, Reverb, Delay, Compressor
*   FX are pre-fader (applied before loop recording)

### **6.4. Waveform Display**

*   Real-time waveform visualization in timeline area (§7.6.6 in main spec)
*   Shows input level during recording
*   Peak level indicator (red if clipping detected)
