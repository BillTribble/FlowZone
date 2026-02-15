import React from 'react';
import { AppState } from '../../../shared/protocol/schema';
import { Fader } from '../components/shared/Fader';
import { Icon } from '../components/shared/Icon';

interface MixerViewProps {
    state: AppState;
    onToggleMetronome: () => void;
}

export const MixerView: React.FC<MixerViewProps> = ({ state, onToggleMetronome }) => {
    // 8 Slots
    // In engine, slots array might be empty initially, so we fill to 8
    const slots = Array.from({ length: 8 }, (_, i) => {
        return state.slots[i] || {
            id: `slot-${i}`,
            state: "EMPTY",
            volume: 0.7,
            name: `Slot ${i + 1}`,
            instrumentCategory: "none"
        };
    });

    return (
        <div style={{ height: '100%', display: 'flex', flexDirection: 'column', padding: 16, gap: 16, boxSizing: 'border-box' }}>
            {/* Top Section: Transport Controls */}
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 8, alignItems: 'center' }}>
                <button
                    onClick={onToggleMetronome}
                    style={{
                        background: state.transport.metronomeEnabled ? '#00E5FF' : '#333',
                        border: 'none', borderRadius: 4, padding: 8,
                        color: state.transport.metronomeEnabled ? '#000' : '#fff',
                        display: 'flex', flexDirection: 'column', alignItems: 'center', cursor: 'pointer'
                    }}>
                    <Icon name="grid" size={16} color={state.transport.metronomeEnabled ? '#000' : '#fff'} />
                    <span style={{ fontSize: 10, marginTop: 4 }}>Quantise</span>
                </button>

                <div style={{ textAlign: 'center' }}>
                    <div style={{ fontSize: 18, fontWeight: 'bold' }}>{state.transport.bpm.toFixed(1)}</div>
                    <div style={{ fontSize: 10, color: '#888' }}>BPM</div>
                </div>

                <button style={{
                    background: '#333', border: 'none', borderRadius: 4, padding: 8, color: '#fff',
                    display: 'flex', flexDirection: 'column', alignItems: 'center', cursor: 'pointer'
                }}>
                    <Icon name="menu" size={16} />
                    <span style={{ fontSize: 10, marginTop: 4 }}>More</span>
                </button>
            </div>

            {/* Middle: Commit Button */}
            <div style={{ display: 'flex', justifyContent: 'center', padding: '10px 0' }}>
                <button style={{
                    background: '#00E5FF', color: '#000', border: 'none', borderRadius: 20,
                    padding: '8px 32px', fontWeight: 'bold', fontSize: 14, cursor: 'pointer',
                    display: 'flex', alignItems: 'center', gap: 8
                }}>
                    <Icon name="play" size={16} color="#000" />
                    COMMIT
                </button>
            </div>

            {/* Bottom: Zigzag Faders */}
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(8, 1fr)',
                gap: 4,
                alignItems: 'flex-end',
                paddingBottom: 20
            }}>
                {slots.map((slot, i) => {
                    // Zigzag: Odd slots (1, 3, 5..) visually higher (i=0, 2, 4..)
                    // Wait, usually Odd slots (1, 3) are top, Even (2, 4) bottom.
                    // Let's toggle margin-bottom based on index.
                    const isEvenIndex = (i % 2 === 0); // 0, 2, 4 -> Slot 1, 3, 5

                    return (
                        <div key={i} style={{
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            marginBottom: isEvenIndex ? 40 : 0
                        }}>
                            <Fader
                                value={slot.volume} // @ts-ignore
                                onChange={(v) => console.log('Vol', i, v)}
                                height={180}
                                width={24}
                                label={(i + 1).toString()}
                            />
                        </div>
                    );
                })}
            </div>
        </div>
    );
};
