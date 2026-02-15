interface HeaderProps {
    sessionName?: string;
    bpm?: number;
    isPlaying?: boolean;
    onTogglePlay?: () => void;
    connected?: boolean;
}

import React from 'react';
import { Icon } from '../shared/Icon';

export const Header: React.FC<HeaderProps> = ({
    sessionName = "Untitled Session",
    bpm = 120.0,
    isPlaying = false,
    onTogglePlay,
    connected = false
}) => {
    return (
        <div
            className="glass-panel"
            style={{
                height: 54,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                padding: '0 16px',
                paddingTop: 'env(safe-area-inset-top)',
                borderRadius: 0,
                borderTop: 'none',
                borderLeft: 'none',
                borderRight: 'none',
                background: 'rgba(10, 10, 11, 0.8)',
                zIndex: 20
            }}
        >
            {/* Left: Home / Connection Status */}
            <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <div style={{
                    width: 8,
                    height: 8,
                    borderRadius: '50%',
                    background: connected ? 'var(--neon-cyan)' : '#f43f5e',
                    boxShadow: connected ? '0 0 10px var(--neon-cyan)' : '0 0 10px #f43f5e'
                }} />
                <span style={{
                    color: 'var(--text-secondary)',
                    fontSize: 10,
                    fontWeight: 900,
                    letterSpacing: '0.1em'
                }}>
                    FLOWZONE
                </span>
            </div>

            {/* Center: Context */}
            <div style={{
                color: '#fff',
                fontSize: 13,
                fontWeight: 600,
                letterSpacing: '0.02em',
                textTransform: 'uppercase'
            }}>
                {sessionName}
            </div>

            {/* Right: Transport */}
            <button
                onClick={onTogglePlay}
                className="interactive-element"
                style={{
                    background: isPlaying ? 'rgba(0, 229, 255, 0.1)' : 'rgba(255, 255, 255, 0.05)',
                    border: `1px solid ${isPlaying ? 'var(--neon-cyan)' : 'var(--glass-border)'}`,
                    borderRadius: 6,
                    color: isPlaying ? 'var(--neon-cyan)' : '#fff',
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    gap: 8,
                    padding: '4px 10px',
                    transition: 'all 0.2s ease'
                }}
            >
                <Icon name={isPlaying ? 'pause' : 'play'} size={14} />
                <span style={{
                    fontSize: 12,
                    width: 36,
                    textAlign: 'right',
                    fontFamily: 'monospace',
                    fontWeight: 'bold'
                }}>
                    {bpm.toFixed(0)}
                </span>
            </button>
        </div>
    );
};
