import React from 'react';

export interface RiffLayer {
    id: string;
    source: 'drums' | 'notes' | 'bass' | 'mic' | 'fx';
    level: number; // 0.0 to 1.0
}

interface RiffIndicatorProps {
    id: string;
    layers: RiffLayer[];
    timestamp: string;
    userName?: string;
    onClick?: (id: string) => void;
    isActive?: boolean;
}

const SOURCE_COLORS: Record<string, string> = {
    drums: '#5E35B1',
    notes: '#00897B',
    bass: '#26A69A',
    mic: '#FFB300',
    fx: '#E65100'
};

export const RiffIndicator: React.FC<RiffIndicatorProps> = ({
    id,
    layers,
    timestamp,
    userName,
    onClick,
    isActive = false
}) => {
    return (
        <div
            onClick={() => onClick?.(id)}
            className={`glass-panel interactive-element ${isActive ? 'neon-glow-cyan' : ''}`}
            style={{
                width: 120,
                height: 48,
                padding: '4px 8px',
                display: 'flex',
                flexDirection: 'column',
                justifyContent: 'space-between',
                cursor: 'pointer',
                border: `1px solid ${isActive ? 'var(--neon-cyan)' : 'var(--glass-border)'}`,
                background: isActive ? 'rgba(0, 229, 255, 0.05)' : 'var(--glass-bg)',
                position: 'relative',
                overflow: 'hidden'
            }}
        >
            {/* Timestamp */}
            <div style={{
                fontSize: '9px',
                color: 'var(--text-secondary)',
                fontFamily: 'monospace',
                display: 'flex',
                justifyContent: 'space-between',
                zIndex: 1
            }}>
                <span>{timestamp}</span>
                {userName && <span style={{ opacity: 0.7 }}>@{userName}</span>}
            </div>

            {/* Layer Cake (Oblong visualization) */}
            <div style={{
                height: 12,
                marginTop: 4,
                display: 'flex',
                gap: 2,
                borderRadius: 4,
                overflow: 'hidden',
                background: 'rgba(0,0,0,0.2)',
                zIndex: 1
            }}>
                {layers.map((layer, i) => (
                    <div
                        key={`${layer.id}-${i}`}
                        style={{
                            flex: layer.level,
                            background: SOURCE_COLORS[layer.source] || '#666',
                            height: '100%',
                            transition: 'flex 0.3s ease',
                            opacity: 0.8,
                            boxShadow: `inset 0 0 4px rgba(0,0,0,0.5)`
                        }}
                    />
                ))}
                {layers.length === 0 && (
                    <div style={{ flex: 1, background: 'rgba(255,255,255,0.05)' }} />
                )}
            </div>

            {/* Active Highlight */}
            {isActive && (
                <div style={{
                    position: 'absolute',
                    left: 0,
                    top: 0,
                    bottom: 0,
                    width: 2,
                    background: 'var(--neon-cyan)'
                }} />
            )}
        </div>
    );
};
