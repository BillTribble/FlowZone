interface HeaderProps {
    sessionName?: string;
    bpm?: number;
    isPlaying?: boolean;
    onTogglePlay?: () => void;
    onPanic?: () => void;
    connected?: boolean;
    onHomeClick?: () => void;
    looperInputLevel?: number;
}

import React, { useRef } from 'react';
import { Icon } from '../shared/Icon';

export const Header: React.FC<HeaderProps> = ({
    sessionName = "Untitled Session",
    bpm = 120.0,
    isPlaying = false,
    onTogglePlay,
    onPanic,
    connected = false,
    onHomeClick,
    looperInputLevel = 0
}) => {
    const longPressTimer = useRef<number | null>(null);
    const [isPressing, setIsPressing] = React.useState(false);

    const handleMouseDown = () => {
        setIsPressing(true);
        longPressTimer.current = setTimeout(() => {
            console.log('[Header] Long press detected - PANIC!');
            onPanic?.();
            setIsPressing(false);
        }, 1000); // 1 second for long press
    };

    const handleMouseUp = () => {
        if (longPressTimer.current) {
            clearTimeout(longPressTimer.current);
            longPressTimer.current = null;
        }
        if (isPressing) {
            // Short press - toggle play
            onTogglePlay?.();
        }
        setIsPressing(false);
    };

    const handleMouseLeave = () => {
        if (longPressTimer.current) {
            clearTimeout(longPressTimer.current);
            longPressTimer.current = null;
        }
        setIsPressing(false);
    };

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
            {/* Left: Home Button + Connection Status */}
            <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <button
                    onClick={onHomeClick}
                    className="interactive-element"
                    title="Jam Manager"
                    style={{
                        background: 'none',
                        border: '1px solid var(--glass-border)',
                        borderRadius: 6,
                        padding: 6,
                        cursor: 'pointer',
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        color: 'var(--text-secondary)',
                        transition: 'all 0.2s ease',
                        width: 32,
                        height: 32
                    }}
                >
                    <Icon name="grid" size={18} color="var(--text-secondary)" />
                </button>
                <div style={{
                    width: 8,
                    height: 8,
                    borderRadius: '50%',
                    background: connected ? 'var(--neon-cyan)' : '#f43f5e',
                    boxShadow: connected ? '0 0 10px var(--neon-cyan)' : '0 0 10px #f43f5e'
                }} />
            </div>

            {/* Looper Input Level Meter */}
            <div style={{
                display: 'flex',
                alignItems: 'center',
                gap: 8
            }}>
                <div style={{
                    fontSize: 9,
                    fontWeight: 700,
                    color: 'var(--text-secondary)',
                    letterSpacing: '0.1em',
                    textTransform: 'uppercase'
                }}>
                    LOOPER IN
                </div>
                <div style={{
                    width: 80,
                    height: 6,
                    background: 'rgba(0,0,0,0.3)',
                    borderRadius: 3,
                    border: '1px solid var(--glass-border)',
                    overflow: 'hidden',
                    position: 'relative'
                }}>
                    <div style={{
                        position: 'absolute',
                        left: 0,
                        top: 0,
                        bottom: 0,
                        width: `${looperInputLevel * 100}%`,
                        background: looperInputLevel > 0.95 ? '#f43f5e' :
                                   looperInputLevel > 0.7 ? '#fbbf24' :
                                   'var(--neon-cyan)',
                        boxShadow: looperInputLevel > 0 ? `0 0 8px ${looperInputLevel > 0.95 ? '#f43f5e' : 'var(--neon-cyan)'}` : 'none',
                        transition: 'width 0.05s linear'
                    }} />
                </div>
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
                onMouseDown={handleMouseDown}
                onMouseUp={handleMouseUp}
                onMouseLeave={handleMouseLeave}
                onTouchStart={handleMouseDown}
                onTouchEnd={handleMouseUp}
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
