import React from 'react';
import { AppState } from '../../../shared/protocol/schema';

interface PlayViewProps {
    state: AppState;
}

// Hardcoded presets for V1 (mirrors Spec)
const PRESETS: Record<string, string[]> = {
    drums: ['Synthetic', 'Lo-Fi', 'Acoustic', 'Electronic'],
    notes: ['Sine Bell', 'Saw Lead', 'Square Bass', 'Triangle Pad', 'Pluck', 'Warm Pad', 'Bright Lead', 'Soft Keys', 'Organ', 'EP', 'Choir', 'Arp'],
    bass: ['Sub', 'Growl', 'Deep', 'Wobble', 'Punch', '808', 'Fuzz', 'Reese', 'Smooth', 'Rumble', 'Pluck Bass', 'Acid'],
    fx: ['Lowpass', 'Highpass', 'Reverb', 'Gate', 'Buzz', 'GoTo', 'Saturator', 'Delay', 'Comb', 'Distortion', 'Smudge', 'Channel'],
    infinite_fx: ['Keymasher', 'Ripper', 'Ringmod', 'Bitcrusher', 'Degrader', 'Pitchmod', 'Multicomb', 'Freezer', 'Zap Delay', 'Dub Delay', 'Compressor', 'Trance Gate']
};

export const PlayView: React.FC<PlayViewProps> = ({ state }) => {
    // Current category from state or default to drums
    const category = state.activeMode?.category || 'drums';
    const presets = PRESETS[category] || [];
    const activePreset = state.activeMode?.presetName;

    return (
        <div style={{
            height: '100%',
            display: 'flex',
            flexDirection: 'column',
            gap: 16,
            padding: 16
        }}>
            {/* 3x4 Preset Grid */}
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(4, 1fr)',
                gridTemplateRows: 'repeat(3, 1fr)',
                gap: 12
            }}>
                {presets.map((preset, i) => {
                    const isActive = preset === activePreset;
                    return (
                        <button
                            key={i}
                            style={{
                                background: isActive ? '#00E5FF' : '#333',
                                color: isActive ? '#000' : '#fff',
                                border: 'none',
                                borderRadius: 8,
                                fontSize: 13,
                                fontWeight: 'bold',
                                cursor: 'pointer',
                                transition: 'all 0.1s ease',
                                boxShadow: isActive ? '0 0 10px rgba(0,229,255,0.5)' : 'none'
                            }}
                            onClick={() => {
                                // TODO: Send SET_PRESET command via WebSocket
                                console.log('Set Preset:', preset);
                            }}
                        >
                            {preset}
                        </button>
                    );
                })}
            </div>
        </div>
    );
};
