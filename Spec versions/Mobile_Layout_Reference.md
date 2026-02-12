# Mobile Layout Reference (JSON)

Summarized from mobile design reference materials.

```json
{
  "mobile_views": {
    "global_header": {
      "top_bar": ["Home", "Help", "Metronome", "Play/Pause", "Loop/Record", "Share", "Add"],
      "context_label": "YOUR SOLO JAM"
    },
    "navigation_tabs": [
      { "id": "instrument", "icon": "grid", "label": "Instrument" },
      { "id": "sound", "icon": "wave", "label": "Sound" },
      { "id": "adjust", "icon": "knob", "label": "Adjust" },
      { "id": "mixer", "icon": "sliders", "label": "Mixer" }
    ],
    "views": {
      "instrument_selector": {
        "categories": [
          { "id": "drums", "label": "Drums", "icon": "drum" },
          { "id": "notes", "label": "Notes", "icon": "note" },
          { "id": "bass", "label": "Bass", "icon": "submarine" },
          { "id": "ext_inst", "label": "Ext Inst", "icon": "keyboard" },
          { "id": "sampler", "label": "Sampler", "icon": "dropper" },
          { "id": "fx", "label": "FX", "icon": "box" },
          { "id": "ext_fx", "label": "Ext FX", "icon": "waveform" },
          { "id": "mic", "label": "Microphone", "icon": "mic" }
        ],
        "preset_grid": ["Slicer", "Razzz", "Acrylic", "Ting", "Hoard", "Bumper", "Amoeba", "Girder", "Demand", "Prey", "Stand", "Lanes"],
        "active_preset": { "name": "Bass", "subtext": "Low End Systems By RedSkyLullaby" }
      },
      "adjust_panel": {
        "controls": [
          { "label": "Pitch", "type": "knob" },
          { "label": "Length", "type": "knob" },
          { "label": "Tone", "type": "knob" },
          { "label": "Level", "type": "knob" },
          { "label": "Bounce", "type": "knob" },
          { "label": "Speed", "type": "knob" },
          { "label": "reserved", "type": "empty" },
          { "label": "Reverb", "type": "knob" }
        ]
      },
      "mixer_session": {
        "transport_grid": [
          { "label": "Quantise", "icon": "note" },
          { "label": "Looper Mode", "icon": "loop" },
          { "label": "More", "icon": "dots" },
          { "label": "Metronome", "icon": "metronome" },
          { "label": "Tempo", "value": "120.00" },
          { "label": "Key", "value": "C Minor Pentatonic" }
        ],
        "primary_actions": ["Start New", "Commit", "Redo"],
        "channel_strips": [
          { "name": "Keymasher", "user": "bill_tribble", "type": "circular_fader" },
          { "name": "Bitcrusher", "user": "bill_tribble", "type": "circular_fader" },
          { "name": "Stark", "user": "bill_tribble", "type": "circular_fader" }
        ]
      },
      "timeline_transport": {
        "toolbar": ["Undo", "Step Indicators (Flowers)", "Expand"],
        "loop_length_selectors": ["+ 8 BARS", "+ 4 BARS", "+ 2 BARS", "+ 1 BAR"],
        "main_grid": "4x4 Pad Grid"
      },
      "riff_history": {
        "header": {
          "user": "bill_tribble",
          "timestamp": "Yesterday",
          "meta": "4/4 120.00 BPM",
          "scale": "C Minor Pentatonic"
        },
        "actions": ["Export Video", "Export Stems", "Delete Rifff"],
        "list": "Chronological grouping by date"
      }
    }
  }
}
```
