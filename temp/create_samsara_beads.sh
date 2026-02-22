#!/bin/bash
id1=$(br create --title="Update Spec and Rename Project to Samsara" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id1 --message "### Detailed Context
Copy archive/v1/Spec/Spec_FlowZone_Looper1.6.md to Spec/Spec_Samsara_Looper2.1.md. Update it to reflect the current JUCE Native architecture (noting React is a future goal). Globally rename the project from FlowZone to Samsara across CMake, src, and build targets."

id2=$(br create --title="Implement Signalsmith Stretch Pitch Shifter" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id2 --message "### Detailed Context
Replace the temporary custom granular pitch engine in StandaloneFXEngine with the Signalsmith-Stretch C++ header. Keep the same XY rules: X is semi-tones (-12 to +12) with a center deadzone, Y is snapped intervals (P4, P5, Octaves) with a center deadzone."

id3=$(br create --title="Implement Disk Writer and Session Hydration" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id3 --message "### Detailed Context
Build a background thread that encodes recorded layers as FLACs to ~/Documents/Samsara/[CurrentGem]/. Maintain a pointer file at the root to tell the app which jam to re-hydrate on startup so sessions persist. Save metadata as session_state.json."

id4=$(br create --title="Implement Blank Canvas (+) Button" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id4 --message "### Detailed Context
Add a '+' button to the top right of the UI. Clicking it pushes a brand new empty Riff to the end of the RiffHistory sequence, allowing the user to begin capturing silently in a fresh state while remaining in the same overall Jam."

echo "Created beads: $id1, $id2, $id3, $id4"
