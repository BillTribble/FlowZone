import React from 'react';
import { AppState } from '../../../shared/protocol/schema';
import { Knob } from '../components/shared/Knob';

interface AdjustViewProps {
    state?: AppState;
}

export const AdjustView: React.FC<AdjustViewProps> = () => {
    // 2 Rows of 4 Knobs
    const knobs = [
        { id: 'pitch', label: 'Pitch', val: 0.5 },
        { id: 'length', label: 'Length', val: 1.0 },
        { id: 'tone', label: 'Tone', val: 0.5 },
        { id: 'level', label: 'Level', val: 0.8 },

        { id: 'bounce', label: 'Bounce', val: 0.0 },
        { id: 'speed', label: 'Speed', val: 0.5 },
        { id: 'reserved', label: '', val: 0, invisible: true },
        { id: 'reverb', label: 'Reverb', val: 0.2 },
    ];

    return (
        <div style={{ height: '100%', display: 'flex', flexDirection: 'column', padding: 20 }}>
            <h2 style={{ fontSize: 16, color: '#888', marginBottom: 20, textAlign: 'center' }}>ADJUST PARAMETERS</h2>

            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(4, 1fr)',
                gridTemplateRows: 'repeat(2, 1fr)',
                gap: 20,
                alignItems: 'center',
                justifyItems: 'center'
            }}>
                {knobs.map((k, i) => {
                    if (k.invisible) return <div key={i} />;
                    return (
                        <Knob
                            key={k.id}
                            label={k.label}
                            value={k.val}
                            onChange={(v) => console.log('Adjust', k.id, v)}
                            size={70}
                            color="#00E5FF"
                        />
                    );
                })}
            </div>

        </div>
    );
};
