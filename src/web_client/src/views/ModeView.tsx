import React from 'react';
import { Icon } from '../components/shared/Icon';

interface ModeViewProps {
    onSelectMode: (mode: string) => void;
}

export const ModeView: React.FC<ModeViewProps> = ({ onSelectMode }) => {
    const categories = [
        { id: 'drums', label: 'DRUMS', icon: 'grid', color: 'var(--neon-blue)' },
        { id: 'notes', label: 'NOTES', icon: 'note', color: 'var(--neon-cyan)' },
        { id: 'bass', label: 'BASS', icon: 'wave', color: 'var(--neon-cyan)' },
        { id: 'ext_inst', label: 'EXT INST', icon: 'settings', color: '#ffb300' },
        { id: 'fx', label: 'FX', icon: 'sliders', color: '#ff7043' },
        { id: 'ext_fx', label: 'EXT FX', icon: 'settings', color: '#ff7043' },
        { id: 'mic', label: 'MIC', icon: 'mic', color: '#ffb300' },
        { id: 'sampler', label: 'SAMPLER', icon: 'grid', disabled: true, color: '#666' }
    ];

    return (
        <div style={{ padding: 20, height: '100%', display: 'flex', flexDirection: 'column' }}>
            <div style={{
                fontSize: 10,
                fontWeight: 900,
                color: 'var(--text-secondary)',
                marginBottom: 24,
                letterSpacing: '0.15em'
            }}>
                SELECT MODE
            </div>
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(auto-fit, minmax(140px, 1fr))',
                gap: 12,
                alignContent: 'start'
            }}>
                {categories.map(cat => (
                    <button
                        key={cat.id}
                        onClick={() => !cat.disabled && onSelectMode(cat.id)}
                        disabled={cat.disabled}
                        className={`glass-panel interactive-element ${!cat.disabled ? 'neon-glow' : ''}`}
                        style={{
                            background: cat.disabled ? 'rgba(0,0,0,0.4)' : 'var(--glass-bg)',
                            border: `1px solid ${cat.disabled ? 'rgba(255,255,255,0.05)' : 'var(--glass-border)'}`,
                            borderRadius: 12,
                            padding: 16,
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            gap: 10,
                            cursor: cat.disabled ? 'default' : 'pointer',
                            opacity: cat.disabled ? 0.3 : 1,
                            minHeight: 90,
                            transition: 'all 0.2s ease'
                        }}
                    >
                        <Icon name={cat.icon} size={24} color={cat.disabled ? '#444' : cat.color} />
                        <span style={{
                            fontSize: 10,
                            fontWeight: 'bold',
                            color: cat.disabled ? '#666' : '#fff',
                            letterSpacing: '0.05em'
                        }}>
                            {cat.label}
                        </span>
                    </button>
                ))}
            </div>
        </div>
    );
};
