import React from 'react';

interface SlotIndicatorProps {
    slotId: number;
    source: 'drums' | 'notes' | 'bass' | 'mic' | 'fx';
    isMuted?: boolean;
    isSelected?: boolean;
    isLooping?: boolean;
    progress?: number; // 0.0 to 1.0
    onClick?: (slotId: number) => void;
    label?: string;
}

const SOURCE_COLORS: Record<string, string> = {
    drums: '#7e57c2', // Deep Purple
    notes: '#26a69a', // Teal
    bass: '#4db6ac', // Lighter Teal/Cyan
    mic: '#ffb300', // Amber
    fx: '#ff7043'    // Deep Orange
};

export const SlotIndicator: React.FC<SlotIndicatorProps> = ({
    slotId,
    source,
    isMuted = false,
    isSelected = false,
    isLooping = false,
    progress = 0,
    onClick,
    label
}) => {
    const color = SOURCE_COLORS[source] || '#666';

    return (
        <div
            onClick={() => onClick?.(slotId)}
            className={`glass-panel interactive-element ${isSelected ? 'neon-glow' : ''}`}
            style={{
                width: 64,
                height: 32,
                padding: '4px 8px',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                cursor: 'pointer',
                border: isSelected ? `2px solid var(--neon-cyan)` : `1px solid ${isMuted ? 'rgba(255,255,255,0.05)' : 'var(--glass-border)'}`,
                background: isMuted ? 'rgba(0,0,0,0.4)' : (isSelected ? 'rgba(0, 229, 255, 0.1)' : 'var(--glass-bg)'),
                position: 'relative',
                overflow: 'hidden',
                opacity: isMuted ? 0.6 : 1
            }}
        >
            {/* Progress Bar Background */}
            {isLooping && !isMuted && (
                <div style={{
                    position: 'absolute',
                    bottom: 0,
                    left: 0,
                    right: 0,
                    height: 3,
                    background: 'rgba(255,255,255,0.1)'
                }} />
            )}

            {/* Progress Bar Fill */}
            {isLooping && !isMuted && (
                <div style={{
                    position: 'absolute',
                    bottom: 0,
                    left: 0,
                    width: `${progress * 100}%`,
                    height: 3,
                    background: color,
                    transition: 'width 0.1s linear',
                    boxShadow: `0 0 8px ${color}`
                }} />
            )}

            {/* Source Dot */}
            <div style={{
                position: 'absolute',
                left: 6,
                top: '50%',
                transform: 'translateY(-50%)',
                width: 6,
                height: 6,
                borderRadius: '50%',
                background: color,
                boxShadow: isMuted ? 'none' : `0 0 6px ${color}`
            }} />

            {/* Label */}
            <span style={{
                fontSize: '11px',
                fontWeight: 'bold',
                color: isSelected ? 'var(--neon-cyan)' : 'var(--text-primary)',
                marginLeft: 10,
                letterSpacing: '0.05em'
            }}>
                {label || `S${slotId + 1}`}
            </span>

            {/* Mute Indicator Overlay */}
            {isMuted && (
                <div style={{
                    position: 'absolute',
                    inset: 0,
                    background: 'rgba(0,0,0,0.2)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontSize: '10px',
                    color: 'rgba(255,255,255,0.3)',
                    fontWeight: 'bold'
                }}>
                    MUTE
                </div>
            )}
        </div>
    );
};
