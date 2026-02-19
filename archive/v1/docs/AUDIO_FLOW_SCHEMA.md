# FlowZone Audio Flow Schema

This document describes the complete audio signal path through FlowZone's engine.

## Audio Flow by Mode

### 1. DRUMS Mode
```
MIDI Input (Pad Triggers)
  ↓
DrumEngine
  ├─ Generate drum sounds (procedural synthesis)
  ├─ Apply ADSR envelopes
  └─ Output stereo audio
        ↓
engineBuffer (temp buffer)
  ↓
RETROSPECTIVE BUFFER ← Capture live drum output ONLY
  ↓
Add to main buffer
  ↓
Mix with Slot Playback
  ↓
Master Output
```

### 2. NOTES/BASS Mode
```
MIDI Input (Pad/Key Triggers)
  ↓
SynthEngine
  ├─ Generate oscillator audio (Sine/Saw/Square/Triangle)
  ├─ Apply tuning (microtuning support)
  ├─ Apply ADSR envelopes
  └─ Output stereo audio
        ↓
engineBuffer (temp buffer)
  ↓
RETROSPECTIVE BUFFER ← Capture live synth output ONLY
  ↓
Add to main buffer
  ↓
Mix with Slot Playback
  ↓
Master Output
```

### 3. MIC Mode
```
Audio Input (Hardware Input Buffer)
  ↓
MicProcessor
  ├─ Apply Input Gain (-60dB to +40dB)
  ├─ Apply Reverb (if enabled)
  └─ Output processed audio
        ↓
engineBuffer (temp buffer)
  ↓
RETROSPECTIVE BUFFER ← Capture processed mic input
  ↓
If Monitor Enabled:
  └─ Add to main buffer (direct monitoring)
  ↓
Mix with Slot Playback
  ↓
Master Output
```

### 4. FX Mode (Future)
```
Selected Slots (1-8)
  ↓
Sum Selected Slots
  ↓
Apply Active FX
  ↓
RETROSPECTIVE BUFFER ← Capture FX output (EXCEPTION: captures slot playback)
  ↓
Add to main buffer
  ↓
Mix with remaining Slot Playback
  ↓
Master Output
```

## Key Principles (Per Spec §3.10)

1. **Retrospective Buffer Captures:**
   - **Normal Modes (Drums/Notes/Bass/Mic):** ONLY live instrument/input audio
   - **Does NOT capture:** Existing slot playback
   - **Prevents:** Audio doubling when layering loops
   - **Exception:** FX Mode captures processed slot audio (by design)

2. **Slot Playback:**
   - Always mixed with live audio
   - Runs independently of retrospective capture
   - Transport-synced playback

3. **Monitor Modes (Mic Only):**
   - **Monitor Input:** Direct monitoring (adds processed input to output)
   - **Monitor Until Looped:** Auto-disables monitoring after first loop capture

## Buffer Flow Summary

```
┌─────────────────────────────────────────────────────────┐
│                   FlowEngine::processBlock              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Process Transport (metronome, phase)                │
│                                                         │
│  2. Generate Live Audio (based on active mode):         │
│     ┌────────────────────────────────────────┐          │
│     │ if (mode == "mic"):                    │          │
│     │   micProcessor.process(input, engine)  │          │
│     │ else if (mode == "drums"):             │          │
│     │   drumEngine.process(engine, midi)     │          │
│     │ else if (mode == "notes"/"bass"):      │          │
│     │   synthEngine.process(engine, midi)    │          │
│     └────────────────────────────────────────┘          │
│                         ↓                                │
│  3. Capture to Retrospective Buffer:                    │
│     retroBuffer.pushBlock(engineBuffer)                 │
│                         ↓                                │
│  4. Add live audio to output:                           │
│     buffer.addFrom(engineBuffer)                        │
│                         ↓                                │
│  5. Mix in Slot Playback:                               │
│     for each slot:                                      │
│       slot.processBlock(buffer) // Adds to mix          │
│                         ↓                                │
│  6. Output to Hardware                                  │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## Critical Implementation Details

### Input Buffer (Hardware → MicProcessor)
- Source: `buffer` parameter in `processBlock()` contains hardware input
- MicProcessor reads FROM this buffer, processes it
- Output goes to `engineBuffer` for further processing

### Engine Buffer (Temporary Processing)
- Created fresh each `processBlock()` call
- Used to isolate live audio generation
- Prevents slot playback from entering retrospective buffer

### Retrospective Capture Buffer
- Circular buffer (~97 seconds at 48kHz)
- Contains ONLY live audio (not slot playback)
- Used for capturing loops via `SET_LOOP_LENGTH`

### Main Output Buffer
- Combines live audio + slot playback
- Goes to hardware output
- In Standalone: Stereo master out
- In VST3: 16 channels (8 stereo pairs, one per slot)

## Visualization Data Flow

```
engineBuffer (Live Audio)
  ↓
FeatureExtractor
  ├─ Peak detection
  ├─ RMS calculation
  └─ Waveform downsampling
        ↓
Binary Stream → WebSocket → React UI → WaveformCanvas
```

The waveform in the UI middle row displays the retrospective buffer content, allowing users to see what will be captured when they tap a loop length.
