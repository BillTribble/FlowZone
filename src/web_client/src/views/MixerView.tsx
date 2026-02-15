import React from 'react';
import { AppState } from '../../../shared/protocol/schema';
import { Fader } from '../components/shared/Fader';
import { Icon } from '../components/shared/Icon';
import { SlotIndicator } from '../components/shared/SlotIndicator';

interface MixerViewProps {
    state: AppState;
    onToggleMetronome: () => void;
    onSlotVolumeChange: (slotIndex: number, volume: number) => void;
    onSelectSlot: (slotIndex: number) => void;
}

export const MixerControls: React.FC<{ state: AppState, onSlotVolumeChange: (slotIndex: number, volume: number) => void }> = ({ state, onSlotVolumeChange }) => {
    // 8 Slots
    const slots = Array.from({ length: 8 }, (_, i) => {
        return state.slots[i] || {
            id: `slot - ${i} `,
            state: "EMPTY",
            volume: 0.7,
            name: `Slot ${i + 1} `,
            instrumentCategory: "none"
        };
    });

    return (
        <div style={{ height: '100%', display: 'flex', flexDirection: 'column', gap: 16 }}>
            {/* Commit Button - Placed at top of bottom controls per request (Controls and Commit in lower half) */}
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

            {/* Faders */}
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(8, 1fr)',
                gap: 4,
                alignItems: 'flex-end',
                paddingBottom: 20
            }}>
                {slots.map((slot, i) => {
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
                                onChange={(v) => onSlotVolumeChange(i, v)}
                                height={140}
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

export const MixerView: React.FC<MixerViewProps> = ({ state, onToggleMetronome }) => {
    return (
        <div style={{ height: '100%', display: 'flex', flexDirection: 'column', padding: 16, gap: 24, boxSizing: 'border-box' }}>
            {/* Top Section: Transport & Quick Settings */}
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 12, alignItems: 'center' }}>
                <button
                    onClick={onToggleMetronome}
                    className="glass-panel interactive-element"
                    style={{
                        background: state.transport.metronomeEnabled ? 'rgba(0, 229, 255, 0.1)' : 'var(--glass-bg)',
                        border: state.transport.metronomeEnabled ? '1px solid var(--neon-cyan)' : '1px solid var(--glass-border)',
                        borderRadius: 8, padding: 12,
                        color: state.transport.metronomeEnabled ? 'var(--neon-cyan)' : '#fff',
                        display: 'flex', flexDirection: 'column', alignItems: 'center', cursor: 'pointer',
                        gap: 6
                    }}>
                    <Icon name="grid" size={16} color={state.transport.metronomeEnabled ? 'var(--neon-cyan)' : '#888'} />
                    <span style={{ fontSize: 9, fontWeight: 'bold' }}>QUANTISET</span>
                </button>

                <div style={{ textAlign: 'center' }}>
                    <div style={{ fontSize: 24, fontWeight: 'bold', letterSpacing: '-0.02em' }}>{state.transport.bpm.toFixed(1)}</div>
                    <div style={{ fontSize: 9, color: 'var(--text-secondary)', fontWeight: 900 }}>BPM</div>
                </div>

                <button
                    className="glass-panel interactive-element"
                    style={{
                        background: 'var(--glass-bg)', border: '1px solid var(--glass-border)', borderRadius: 8, padding: 12, color: '#fff',
                        display: 'flex', flexDirection: 'column', alignItems: 'center', cursor: 'pointer',
                        gap: 6
                    }}>
                    <Icon name="settings" size={16} color="#888" />
                    <span style={{ fontSize: 9, fontWeight: 'bold' }}>MORE</span>
                </button>
            </div>

            {/* Slot Layout Grid (Top half visualization) */}
            <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: 12 }}>
                <div style={{ fontSize: 10, fontWeight: 900, color: 'var(--text-secondary)', letterSpacing: '0.1em' }}>
                    SLOT STATUS
                </div>
                <div style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(4, 1fr)',
                    gap: 10,
                    gridAutoRows: 'auto'
                }}>
                    {state.slots.map((slot, i) => (
                        <SlotIndicator
                            key={slot.id}
                            slotId={i}
                            source={slot.instrumentCategory as any || 'notes'}
                            isMuted={slot.state === 'EMPTY'}
                            isSelected={false}
                            isLooping={false}
                            progress={0}
                            label={slot.name}
                        />
                    ))}
                </div>
            </div>
        </div>
    );
};
