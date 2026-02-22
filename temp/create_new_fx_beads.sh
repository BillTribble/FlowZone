#!/bin/bash
id1=$(br create --title="Rationalise FX List" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id1 --message "### Detailed Context
Remove Distortion, Ripper, Channel, and Compress. Rename Saturator to Distort. Update both UI arrays and StandaloneFXEngine."

id2=$(br create --title="Implement Pattern-based TrGate" --type=task --priority=2 | grep -o 'bd-[a-zA-Z0-9]*')
br comments add $id2 --message "### Detailed Context
Change TrGate from LFO-driven to pattern-based (classic trance gate). Define 16-step sequences. X axis selects pattern, Y axis controls depth/mix."

echo "Created beads: $id1, $id2"
