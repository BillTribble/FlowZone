import React from 'react';
import { Knob } from '../components/shared/Knob';

interface AdjustViewProps {
    onAdjustParam?: (id: string, value: number) => void;
}

export const AdjustView: React.FC<AdjustViewProps> = ({ onAdjustParam }) => {
    const knobs = [
        { id: 'pitch', label: 'PITCH', val: 0.5 },
        { id: 'length', label: 'LENGTH', val: 1.0 },
        { id: 'tone', label: 'TONE', val: 0.5 },
        { id: 'level', label: 'LEVEL', val: 0.8 },

        { id: 'bounce', label: 'BOUNCE', val: 0.0 },
        { id: 'speed', label: 'SPEED', val: 0.5 },
        { id: 'reserved', label: '', val: 0, invisible: true },
        { id: 'reverb', label: 'REVERB', val: 0.2 },
    ];

    return (
        <div style={{ height: '100%', display: 'flex', flexDirection: 'column', padding: 24, gap: 20 }}>
            <div style={{
                fontSize: 10,
                fontWeight: 900,
                color: 'var(--text-secondary)',
                marginBottom: 10,
                letterSpacing: '0.15em',
                textAlign: 'center'
            }}>
                ADJUST PARAMETERS
            </div>

            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(4, 1fr)',
                gridTemplateRows: 'repeat(2, 1fr)',
                gap: 20,
                alignItems: 'center',
                justifyItems: 'center',
                padding: '10px 0'
            }}>
                {knobs.map((k, i) => {
                    if (k.invisible) return <div key={i} />;
                    return (
                        <div key={k.id} className="interactive-element" style={{
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            gap: 8
                        }}>
                            <Knob
                                label={k.label}
                                value={k.val}
                                onChange={(v) => onAdjustParam?.(k.id, v)}
                                size={64}
                                color="var(--neon-cyan)"
                            />
                        </div>
                    );
                })}
            </div>
        </div>
    );
};
