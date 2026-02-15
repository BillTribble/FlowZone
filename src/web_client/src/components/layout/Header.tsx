import React from 'react';
import { Icon } from '../shared/Icon';

interface HeaderProps {
    sessionName?: string;
    bpm?: number;
    isPlaying?: boolean;
    onTogglePlay?: () => void;
    connected?: boolean;
}

export const Header: React.FC<HeaderProps> = ({
    sessionName = "Untitled Session",
    bpm = 120.0,
    isPlaying = false,
    onTogglePlay,
    connected = false
}) => {
    return (
        <div style={{
            height: 50,
            background: '#1a1a1a',
            borderBottom: '1px solid #333',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            padding: '0 16px',
            paddingTop: 'env(safe-area-inset-top)'
        }}>
            {/* Left: Home / Connection Status */}
            <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
                <div style={{
                    width: 8,
                    height: 8,
                    borderRadius: '50%',
                    background: connected ? '#0f0' : '#f00',
                    boxShadow: connected ? '0 0 5px #0f0' : 'none'
                }} />
                <span style={{ color: '#888', fontSize: 12 }}>HOME</span>
            </div>

            {/* Center: Context */}
            <div style={{ color: '#fff', fontSize: 14, fontWeight: 'bold' }}>
                {sessionName}
            </div>

            {/* Right: Transport */}
            <button
                onClick={onTogglePlay}
                style={{
                    background: 'none',
                    border: 'none',
                    color: isPlaying ? '#00E5FF' : '#fff',
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    gap: 8
                }}
            >
                <Icon name={isPlaying ? 'pause' : 'play'} size={20} />
                <span style={{ fontSize: 12, width: 40, textAlign: 'right' }}>{bpm.toFixed(1)}</span>
            </button>
        </div>
    );
};
