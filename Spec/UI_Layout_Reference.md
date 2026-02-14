# UI Layout Reference (JSON)

Summarized from mobile design reference materials.

```json
{
  "ui_layout_specification": {
    "global_elements": {
      "header": {
        "context_label": {
          "content": "[random_emoji] [date_simple]",
          "position": "top_center"
        },
        "top_bar_controls": {
          "left_section": ["Home"],
          "center_section": ["Metronome", "Play/Pause"],
          "right_section": []
        }
      },
      "navigation_tabs": {
        "layout": "horizontal_bar",
        "position": "bottom_of_content_area",
        "tabs": [
          { "id": "mode", "icon": "grid", "label": "Mode", "note": "Primary mode selector. Instantly switches to Play tab on selection." },
          { "id": "play", "icon": "wave", "label": "Play", "note": "Primary performative view. Shows presets + pads for selected category." },
          { "id": "adjust", "icon": "knob", "label": "Adjust" },
          { "id": "mixer", "icon": "sliders", "label": "Mixer" }
        ]
      },
      "riff_history_indicators": {
        "description": "Riffs displayed right-to-left in history with latest on right",
        "display_format": "oblong_layer_cake",
        "format_details": "Oblong layer cake with rounded edges, layers stacked vertically. Each color represents the input source. Multiple layers of same color differentiate with 30% brightness increase/decrease.",
        "interaction": "tappable to jump backwards in riff history",
        "behavior": {
          "playback_change": "configurable in settings",
          "options": ["instant", "swap_on_bar"]
        },
        "position": "below_navigation_tabs"
      },
      "timeline_area": {
        "retrospective_timeline": {
          "flow_direction": "right_to_left",
          "sections": [
            { "length": "1_bar", "position": "rightmost_section" },
            { "length": "2_bars", "position": "second_from_right" },
            { "length": "4_bars", "position": "third_from_right" },
            { "length": "8_bars", "position": "leftmost_section" }
          ],
          "note": "Waveform sections act as buttons. Labels (8 BARS, etc.) overlay the sections.",
          "interaction": "tap specific waveform section to loop that length AND capture audio",
          "action": "absolute_set_loop_length"
        },
        "toolbar": {
          "items": ["Riff History Indicators", "Expand"],
          "position": "between_tabs_and_waveform"
        }
      }
    },
    "views": {
      "instrument_tab": {
        "category_selector": {
          "layout": "2x4_grid",
          "position": "top_section",
          "note": "Mode tab is purely for category selection. No presets/pads shown here.",
          "categories": [
            { "id": "drums", "label": "Drums", "icon": "drum", "row": 1, "col": 1 },
            { "id": "notes", "label": "Notes", "icon": "note", "row": 1, "col": 2 },
            { "id": "bass", "label": "Bass", "icon": "submarine", "row": 1, "col": 3 },
            { "id": "ext_inst", "label": "Ext Inst", "icon": "keyboard", "row": 1, "col": 4, "placeholder": "Coming Soon" },
            { "id": "fx", "label": "FX", "icon": "box", "row": 2, "col": 1 },
            { "id": "ext_fx", "label": "Ext FX", "icon": "waveform", "row": 2, "col": 2, "placeholder": "Coming Soon" },
            { "id": "mic", "label": "Microphone", "icon": "mic", "row": 2, "col": 3 },
            { "id": "reserved", "label": "", "row": 2, "col": 4, "note": "Reserved for V2 Sampler" }
          ]
        },
        "active_preset_display": {
          "position": "top_right",
          "shows": ["preset_name", "creator_attribution"]
        },
        "pad_grid": {
          "structure": "4x4_grid",
          "position": "bottom_section",
          "content_type": "varies_by_instrument",
          "examples": {
            "drums": {
              "cell_content": "drum_icons",
              "samples": [
                { "row": 1, "col": 1, "icon": "double_diamond" },
                { "row": 1, "col": 2, "icon": "cylinder" },
                { "row": 1, "col": 3, "icon": "tall_cylinder" },
                { "row": 1, "col": 4, "icon": "tripod" },
                { "row": 2, "col": 1, "icon": "double_diamond_outline" },
                { "row": 2, "col": 2, "icon": "double_diamond_dotted" },
                { "row": 2, "col": 3, "icon": "double_diamond_striped" },
                { "row": 2, "col": 4, "icon": "cylinder_short" },
                { "row": 3, "col": 1, "icon": "hand" },
                { "row": 3, "col": 2, "icon": "hand" },
                { "row": 3, "col": 3, "icon": "snare" },
                { "row": 3, "col": 4, "icon": "snare" },
                { "row": 4, "col": 1, "icon": "lollipop" },
                { "row": 4, "col": 2, "icon": "lollipop" },
                { "row": 4, "col": 3, "icon": "lollipop" },
                { "row": 4, "col": 4, "icon": "lollipop" }
              ]
            },
            "notes_bass": {
              "cell_content": "colored_pads",
              "note": "pads colored based on instrument theme. Root note (octaves) pads are brighter."
            }
          }
        },
        "preset_selector": {
          "layout": "3x4_grid",
          "position": "middle_section",
          "preset_examples": "See Audio_Engine_Specifications.md for preset names per category. FlowZone-specific names TBD.",
          "selected_state": "highlighted_with_rounded_background"
        },
        "add_plugin": {
          "trigger": "category_with_plus_button",
          "examples": ["Plugin Effects", "Plugin Instrument"],
          "modal": {
            "icon": "plug",
            "text": "Add a new Plugin",
            "style": "centered_card_with_border"
          }
        }
      },
      "play_tab": {
        "note": "Primary performative view. Instantly switches from Mode tab. Shows presets + pads. XY pad visible only in FX Mode.",
        "preset_selector": {
          "layout": "3x4_grid",
          "position": "top_section",
          "active_preset_display": "top_with_attribution",
          "preset_examples": {
            "fx": ["Lowpass", "Highpass", "Reverb", "Gate", "Buzz", "GoTo", "Saturator", "Delay", "Comb", "Distortion", "Smudge", "Channel"],
            "infinite_fx": ["Keymasher", "Ripper", "Ringmod", "Bitcrusher", "Degrader", "Pitchmod", "Multicomb", "Freezer", "Zap Delay", "Dub Delay", "Compressor"]
          }
        },
        "effect_control_area": {
          "keymasher": {
            "layout": "3x4_button_grid",
            "position": "below_timeline",
            "buttons": [
              { "row": 1, "col": 1, "label": "Repeat" },
              { "row": 1, "col": 2, "label": "Pitch Down" },
              { "row": 1, "col": 3, "label": "Pitch Rst" },
              { "row": 1, "col": 4, "label": "Pitch Up" },
              { "row": 2, "col": 1, "label": "Reverse" },
              { "row": 2, "col": 2, "label": "Gate" },
              { "row": 2, "col": 3, "label": "Scratch" },
              { "row": 2, "col": 4, "label": "Buzz" },
              { "row": 3, "col": 1, "label": "Stutter" },
              { "row": 3, "col": 2, "label": "Go To" },
              { "row": 3, "col": 3, "label": "Go To 2" },
              { "row": 3, "col": 4, "label": "Buzz slip" }
            ],
            "slot_indicators": {
              "layout": "1_row_of_8_oblongs",
              "position": "below_button_grid",
              "note": "Oblong slot indicators. In FX mode: toggle FX source selection. In other modes: toggle mute/unmute."
            }
          },
          "other_effects": {
            "layout": "xy_pad",
            "description": "XY control pad with crosshair",
            "behavior": {
              "crosshair_display": "appears only when finger held down",
              "effect_active": "only while finger held down",
              "interaction_style": "highly playable touch-and-hold effect"
            },
            "visual": "large rectangular pad with dotted crosshair guides",
            "slot_indicators": {
              "layout": "1_row_of_8_oblongs",
              "position": "below_xy_pad",
              "note": "Oblong slot indicators. In FX mode: toggle FX source selection. In other modes: toggle mute/unmute."
            }
          }
        }
      },
      "adjust_tab": {
        "knob_controls": {
          "layout": "2_rows_of_4_positions",
          "structure": [
            { "row": 1, "col": 1, "label": "Pitch", "type": "knob" },
            { "row": 1, "col": 2, "label": "Length", "type": "knob" },
            { "row": 1, "col": 3, "label": "Tone", "type": "knob" },
            { "row": 1, "col": 4, "label": "Level", "type": "knob" },
            { "row": 2, "col": 1, "label": "Bounce", "type": "knob" },
            { "row": 2, "col": 2, "label": "Speed", "type": "knob" },
            { "row": 2, "col": 3, "label": "reserved", "type": "empty" },
            { "row": 2, "col": 4, "label": "Reverb", "type": "knob" }
          ],
          "note": "In Drum Mode, Bounce and Speed knobs are Hidden/Disabled.",
          "additional_knobs": {
            "reverb_section": [
              { "label": "Reverb Mix", "type": "knob" },
              { "label": "Room Size", "type": "knob" }
            ],
            "layout_note": "Additional knobs displayed in similar 2-row grid below main controls"
          }
        },
        "pad_grid": {
          "structure": "4x4_grid",
          "position": "below_knobs",
          "content": "instrument_specific_pads"
        }
      },
      "mixer_tab": {
        "transport_controls": {
          "layout": "2x3_grid",
          "position": "top_section",
          "controls": [
            { "row": 1, "col": 1, "label": "Quantise", "icon": "note", "type": "button" },
            { "row": 1, "col": 3, "label": "More", "icon": "dots", "type": "button" },
            { "row": 2, "col": 1, "label": "Metronome", "icon": "metronome", "type": "toggle" },
            { "row": 2, "col": 2, "label": "Tempo", "value": "120.00", "type": "display_button" },
            { "row": 2, "col": 3, "label": "Key", "value": "C Minor Pentatonic", "type": "display_button" }
          ]
        },
        "primary_actions": {
          "layout": "horizontal_row",
          "position": "middle_section",
          "buttons": [
            { "label": "Commit", "icon": "checkmark", "style": "light_prominent" }
          ]
        },
        "channel_strips": {
          "layout": "vertical_fader_strips",
          "arrangement": "scales_to_fit_screen",
          "note": "No horizontal scrolling required for V1.",
          "channels_note": "Channel strip names shown below are illustrative — see Audio_Engine_Specifications.md for actual preset names.",
          "channels": [
            { "name": "Saw Lead", "user": "bill_tribble", "type": "vertical_fader" },
            { "name": "Dub Delay", "user": "bill_tribble", "type": "vertical_fader" },
            { "name": "Sub", "user": "bill_tribble", "type": "vertical_fader" }
          ],
          "fader_style": "vertical_slider_with_integrated_vu_meter",
          "fader_note": "VU level bounces inside the fader track.",
          "display_info": ["instrument_name", "user_attribution"]
        }
      },
      "microphone_tab": {
        "category_selector": "same_as_instrument_tab",
        "microphone_selected_state": "highlighted",
        "controls": {
          "monitor_settings": [
            { "label": "Monitor until looped", "type": "toggle", "default": "off" },
            { "label": "Monitor input", "type": "toggle", "default": "off" }
          ],
          "gain_control": {
            "type": "large_circular_knob",
            "label": "Gain",
            "position": "center_bottom",
            "style": "prominent_with_arc_indicator"
          }
        },
        "timeline_display": {
          "shows": "recording_waveform",
          "note": "displays audio input waveform during recording"
        }
      },
      "riff_history_view": {
        "header": {
          "back_button": "top_left",
          "transport_controls": "center (metronome, play/pause)"
        },
        "riff_details": {
          "user_info": "bill_tribble",
          "timestamp": "Yesterday",
          "metadata": "4/4 120.00 BPM",
          "scale": "C Minor Pentatonic",
          "avatar": "circular_user_image",
          "riff_icon": "oblong_layer_cake_large"
        },
        "actions": {
          "layout": "horizontal_row",
          "buttons": ["Delete Riff", {"label": "Export Stems", "disabled": true, "note": "Coming Soon — V2"}]
        },
        "history_list": {
          "grouping": "chronological_by_date",
          "date_headers": ["11 Feb 2026", "7 Feb 2026"],
          "riff_items": {
            "layout": "grid",
            "display": "oblong_layer_cake_with_user_badge",
            "user_badge": "circular_badge_with_initial",
            "selection_state": "outlined_border",
            "interaction": "tappable_to_load_riff"
          },
          "load_control": {
            "type": "button",
            "label": "Load Older",
            "position": "bottom_center"
          }
        }
      },
      "settings_panel": {
        "note": "Sole settings access point on all devices. Opened from More button in Mixer tab.",
        "header": {
          "title": "Settings",
          "close_button": "top_right_x"
        },
        "tabs": [
          {
            "id": "interface",
            "label": "Interface",
            "controls": [
              { "label": "Zoom Level", "type": "slider", "range": "50%-200%", "default": "100%" },
              { "label": "Theme", "type": "selector", "options": ["Dark", "Mid", "Light", "Match System"] },
              { "label": "Font Size", "type": "selector", "options": ["Small", "Medium", "Large"], "default": "Medium" },
              { "label": "Reduce Motion", "type": "toggle", "default": "off" },
              { "label": "Emoji Skin Tone", "type": "selector", "options": ["None", "Light", "Medium-Light", "Medium", "Medium-Dark", "Dark"] }
            ]
          },
          {
            "id": "audio",
            "label": "Audio",
            "controls": [
              { "label": "Driver Type", "type": "display", "value": "CoreAudio" },
              { "label": "Input Device", "type": "dropdown" },
              { "label": "Output Device", "type": "dropdown" },
              { "label": "Sample Rate", "type": "dropdown", "options": ["44.1kHz", "48kHz", "88.2kHz", "96kHz"] },
              { "label": "Buffer Size", "type": "dropdown", "options": ["16", "32", "64", "128", "256", "512", "1024"] },
              { "label": "Input Channels", "type": "checkbox_matrix" },
              { "label": "Active Output Channels", "type": "checkbox_list" },
              { "label": "Test", "type": "button", "action": "play_test_tone" }
            ]
          },
          {
            "id": "midi_sync",
            "label": "MIDI & Sync",
            "controls": [
              { "label": "MIDI Inputs", "type": "checkbox_list" },
              { "label": "Clock Source", "type": "radio", "options": ["Internal", "External MIDI Clock"] },
              { "label": "Ableton Link", "type": "toggle", "default": "off", "disabled": true, "note": "Coming Soon (V2)" }
            ]
          },
          {
            "id": "library_vst",
            "label": "Library & VST",
            "controls": [
              { "label": "VST3 Search Paths", "type": "path_list", "actions": ["Add", "Remove"] },
              { "label": "Rescan All Plugins", "type": "button" },
              { "label": "Scan on Startup", "type": "toggle" },
              { "label": "Storage Location", "type": "path_selector" }
            ]
          }
        ],
        "quick_toggles": {
          "position": "below_tabs",
          "always_visible": true,
          "options": [
            { "label": "Note Names", "type": "toggle", "default": "off", "command": "TOGGLE_NOTE_NAMES" }
          ]
        },
        "user_preferences": {
          "position": "below_quick_toggles",
          "always_visible": true,
          "options": [
            { "label": "Riff Swap Mode", "type": "radio", "options": ["Instant", "Swap on Bar"], "default": "Instant", "command": "SET_RIFF_SWAP_MODE" }
          ]
        }
      }
    }
  }
}
```
