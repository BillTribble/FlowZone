import React from 'react';
import { Icon } from '../components/shared/Icon';

interface ModeViewProps {
    onSelectMode: (mode: string) => void;
}

export const ModeView: React.FC<ModeViewProps> = ({ onSelectMode }) => {
    const categories = [
        { id: 'drums', label: 'Drums', icon: 'grid' }, // using grid as drum icon placeholder
        { id: 'notes', label: 'Notes', icon: 'note' },
        { id: 'bass', label: 'Bass', icon: 'wave' },
        { id: 'ext_inst', label: 'Ext Inst', icon: 'settings' }, // placeholder
        { id: 'fx', label: 'FX', icon: 'sliders' },
        { id: 'ext_fx', label: 'Ext FX', icon: 'settings' },
        { id: 'mic', label: 'Mic', icon: 'mic' },
        { id: 'sampler', label: 'Sampler', icon: 'grid', disabled: true }
    ];

    return (
        <div style={{ padding: 16 }}>
            <h2 style={{ fontSize: 16, color: '#888', marginBottom: 20 }}>SELECT MODE</h2>
            <div style={{
                display: 'grid',
                gridTemplateColumns: 'repeat(4, 1fr)', // Change to 4 columns to fit better
                gap: 8
            }}>
                {categories.map(cat => (
                    <button
                        key={cat.id}
                        onClick={() => !cat.disabled && onSelectMode(cat.id)}
                        disabled={cat.disabled}
                        style={{
                            background: '#333',
                            border: '1px solid #444',
                            borderRadius: 8,
                            padding: 12, // Reduced padding
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            gap: 8,
                            cursor: cat.disabled ? 'default' : 'pointer',
                            opacity: cat.disabled ? 0.3 : 1,
                            minHeight: 80
                        }}
                    >
                        <Icon name={cat.icon} size={32} color={cat.disabled ? '#666' : '#fff'} />
                        <span style={{ fontSize: 14, fontWeight: 'bold' }}>{cat.label}</span>
                    </button>
                ))}
            </div>
        </div>
    );
};
