import React from 'react';
import { AppState } from '../../../shared/protocol/schema';

interface PlayViewProps {
    state: AppState;
    onSelectPreset: (category: string, preset: string) => void;
    selectedPreset?: string | null;
}

// Hardcoded presets for V1 (mirrors Spec)
const PRESETS: Record<string, string[]> = {
    drums: ['Synthetic', 'Lo-Fi', 'Acoustic', 'Electronic'],
    notes: ['Sine Bell', 'Saw Lead', 'Square Bass', 'Triangle Pad', 'Pluck', 'Warm Pad', 'Bright Lead', 'Soft Keys', 'Organ', 'EP', 'Choir', 'Arp'],
    bass: ['Sub', 'Growl', 'Deep', 'Wobble', 'Punch', '808', 'Fuzz', 'Reese', 'Smooth', 'Rumble', 'Pluck Bass', 'Acid'],
    fx: ['Lowpass', 'Highpass', 'Reverb', 'Gate', 'Buzz', 'GoTo', 'Saturator', 'Delay', 'Comb', 'Distortion', 'Smudge', 'Channel'],
    infinite_fx: ['Keymasher', 'Ripper', 'Ringmod', 'Bitcrusher', 'Degrader', 'Pitchmod', 'Multicomb', 'Freezer', 'Zap Delay', 'Dub Delay', 'Compressor', 'Trance Gate']
};

export const PlayView: React.FC<PlayViewProps> = ({ state, onSelectPreset, selectedPreset }) => {
    // Current category from state or default to drums
    const category = state.activeMode?.category || 'drums';
    const presets = PRESETS[category] || [];
    const activePreset = selectedPreset || state.activeMode?.presetName;

    return (
        <div style={{
            height: '100%',
            display: 'flex',
            flexDirection: 'column',
            gap: 20,
            padding: 24
        }}>
            <div style={{
                fontSize: 10,
                fontWeight: 900,
                color: 'var(--text-secondary)',
                letterSpacing: '0.15em',
                textAlign: 'center'
            }}>
                {category.toUpperCase()} PRESETS
            </div>

            {/* 3x4 Preset Grid */}
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(auto-fit, minmax(120px, 1fr))',
                gridAutoRows: '1fr',
                gap: 12,
                alignContent: 'start'
            }}>
                {presets.map((preset, i) => {
                    const isActive = preset === activePreset;
                    return (
                        <button
                            key={i}
                            className={`glass-panel interactive-element ${isActive ? 'neon-glow' : ''}`}
                            style={{
                                background: isActive ? 'rgba(0, 229, 255, 0.2)' : 'var(--glass-bg)',
                                border: isActive ? '2px solid var(--neon-cyan)' : '1px solid var(--glass-border)',
                                color: isActive ? 'var(--neon-cyan)' : 'var(--text-primary)',
                                borderRadius: 8,
                                fontSize: 11,
                                fontWeight: isActive ? 900 : 'bold',
                                cursor: 'pointer',
                                transition: 'all 0.2s ease',
                                textTransform: 'uppercase',
                                letterSpacing: '0.02em',
                                boxShadow: isActive ? '0 0 20px rgba(0, 229, 255, 0.3), inset 0 0 10px rgba(0, 229, 255, 0.1)' : 'none',
                                transform: isActive ? 'scale(1.02)' : 'scale(1)',
                                minHeight: 60
                            }}
                            onClick={() => {
                                onSelectPreset(category, preset);
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
