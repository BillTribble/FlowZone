#!/bin/bash
id1=$(br create --title="Update FX Page Layout" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id1 --message "### Detailed Context
Refactor the FXTabTop arrays:
Move Keymasher and Pitchmod to CoreFX. Move Buzz to InfFX, placing Dub Delay on CoreFX. Fix the StandaloneFXEngine switch cases to match."

id2=$(br create --title="Implement Granular Pitch Shifter for Pitchmod" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id2 --message "### Detailed Context
Replace the old Pitchmod ringmod code with a true time-preserving Pitch Shifter using granular synthesis.
X-axis: Smooth semitones (-12 to +12, wide center deadzone).
Y-axis: Snapped intervals (-24, -19, -17, -12, -7, -5, 0, 5, 7, 12, 17, 19, 24).
Combine X and Y axis values."

echo "Created beads: $id1, $id2"
