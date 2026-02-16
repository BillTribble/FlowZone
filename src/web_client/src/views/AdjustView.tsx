import React from 'react';
import { Knob } from '../components/shared/Knob';

interface AdjustViewProps {
    onAdjustParam?: (id: string, value: number) => void;
    activeMode: string;
}

export const AdjustView: React.FC<AdjustViewProps> = ({ onAdjustParam, activeMode }) => {
    // Mic mode has different controls per spec §7.6.4
    if (activeMode === 'mic') {
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
                    MICROPHONE CONTROLS
                </div>

                {/* Reverb Controls */}
                <div style={{
                    display: 'flex',
                    justifyContent: 'center',
                    gap: 40,
                    marginBottom: 20
                }}>
                    <div className="interactive-element" style={{
                        display: 'flex',
                        flexDirection: 'column',
                        alignItems: 'center',
                        gap: 8
                    }}>
                        <Knob
                            label="REVERB MIX"
                            value={0.3}
                            onChange={(v) => onAdjustParam?.('reverb_mix', v)}
                            size={64}
                            color="var(--neon-cyan)"
                        />
                    </div>
                    <div className="interactive-element" style={{
                        display: 'flex',
                        flexDirection: 'column',
                        alignItems: 'center',
                        gap: 8
                    }}>
                        <Knob
                            label="ROOM SIZE"
                            value={0.5}
                            onChange={(v) => onAdjustParam?.('room_size', v)}
                            size={64}
                            color="var(--neon-cyan)"
                        />
                    </div>
                </div>

                {/* Large Gain Knob */}
                <div style={{
                    flex: 1,
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    justifyContent: 'center',
                    gap: 20
                }}>
                    <div className="interactive-element">
                        <Knob
                            label="GAIN"
                            value={0.7}
                            onChange={(v) => onAdjustParam?.('gain', v)}
                            size={120}
                            color="var(--neon-cyan)"
                        />
                    </div>
                    <div style={{ fontSize: 11, color: 'var(--text-secondary)' }}>
                        -60dB to +40dB
                    </div>
                </div>
            </div>
        );
    }

    // Standard instrument knobs
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
