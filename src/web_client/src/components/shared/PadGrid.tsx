import React from 'react';
import { Pad } from './Pad';

interface PadGridProps {
    baseNote: number; // 48 for C3, 36 for C2
    scale: string; // 'major', 'minor', etc.
    activePads: Set<number>;
    onPadDown: (midiNote: number, padId: number) => void;
    onPadUp: (midiNote: number, padId: number) => void;
}

const SCALE_INTERVALS: Record<string, number[]> = {
    major: [0, 2, 4, 5, 7, 9, 11],
    minor: [0, 2, 3, 5, 7, 8, 10],
    minor_pentatonic: [0, 3, 5, 7, 10],
    major_pentatonic: [0, 2, 4, 7, 9],
    dorian: [0, 2, 3, 5, 7, 9, 10],
    mixolydian: [0, 2, 4, 5, 7, 9, 10],
    blues: [0, 3, 5, 6, 7, 10],
    chromatic: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
};

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

export const PadGrid: React.FC<PadGridProps> = ({
    baseNote,
    scale,
    activePads,
    onPadDown,
    onPadUp
}) => {
    const intervals = SCALE_INTERVALS[scale] || SCALE_INTERVALS.major;
    const numSteps = intervals.length;

    const getMidiNote = (padId: number) => {
        const octave = Math.floor(padId / numSteps);
        const degree = padId % numSteps;
        return baseNote + (octave * 12) + intervals[degree];
    };

    const isRoot = (padId: number) => (padId % numSteps) === 0;

    const getNoteLabel = (midiNote: number) => {
        const name = NOTE_NAMES[midiNote % 12];
        const oct = Math.floor(midiNote / 12) - 1;
        return `${name}${oct}`;
    };

    return (
        <div style={{
            height: '100%',
            width: '100%',
            display: 'grid',
            gridTemplateColumns: 'repeat(4, 1fr)',
            gridTemplateRows: 'repeat(4, 1fr)',
            gap: 12,
            padding: 12
        }}>
            {Array.from({ length: 16 }, (_, i) => {
                const row = Math.floor(i / 4);
                const col = i % 4;
                const visualPadId = (3 - row) * 4 + col; // 0 at bottom left

                const midiNote = getMidiNote(visualPadId);

                return (
                    <Pad
                        key={i}
                        label={getNoteLabel(midiNote)}
                        active={activePads.has(visualPadId)}
                        isRoot={isRoot(visualPadId)}
                        onMouseDown={() => onPadDown(midiNote, visualPadId)}
                        onMouseUp={() => onPadUp(midiNote, visualPadId)}
                    />
                );
            })}
        </div>
    );
};
