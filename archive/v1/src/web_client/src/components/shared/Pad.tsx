import React from 'react';

interface PadProps {
    label?: string;
    active?: boolean;
    isRoot?: boolean;
    color?: string;
    onClick?: () => void;
    onMouseDown?: () => void;
    onMouseUp?: () => void;
    icon?: React.ReactNode;
}

export const Pad: React.FC<PadProps> = ({
    label,
    active = false,
    isRoot = false,
    color = 'var(--neon-cyan)',
    onClick,
    onMouseDown,
    onMouseUp,
    icon
}) => {
    return (
        <button
            onClick={onClick}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            onMouseLeave={onMouseUp}
            onTouchStart={onMouseDown}
            onTouchEnd={onMouseUp}
            className={`interactive-element ${active ? 'neon-glow' : ''}`}
            style={{
                width: '100%',
                height: '100%',
                aspectRatio: '1/1',
                borderRadius: 8,
                backgroundColor: active ? color : (isRoot ? 'rgba(255,255,255,0.05)' : 'rgba(255,255,255,0.02)'),
                border: active ? `2px solid ${color}` : `1px solid ${isRoot ? 'rgba(255,255,255,0.2)' : 'rgba(255,255,255,0.05)'}`,
                boxShadow: active ? `0 0 20px ${color}` : 'none',
                cursor: 'pointer',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                gap: 4,
                position: 'relative',
                overflow: 'hidden',
                transition: 'all 0.1s cubic-bezier(0.4, 0, 0.2, 1)',
                backdropFilter: 'blur(4px)',
                boxSizing: 'border-box'
            }}
        >
            {/* Inner Glow */}
            {active && (
                <div style={{
                    position: 'absolute',
                    inset: 0,
                    background: color,
                    opacity: 0.2,
                    pointerEvents: 'none'
                }} />
            )}

            {icon && (
                <div style={{
                    color: active ? '#000' : (isRoot ? '#fff' : '#888'),
                    transform: active ? 'scale(0.9)' : 'scale(1)',
                    transition: 'all 0.1s ease'
                }}>
                    {icon}
                </div>
            )}

            {label && (
                <span style={{
                    fontSize: '11px',
                    color: active ? '#000' : (isRoot ? '#fff' : '#64748b'),
                    fontWeight: isRoot || active ? 'bold' : 'normal',
                    pointerEvents: 'none',
                    letterSpacing: '0.05em'
                }}>
                    {label}
                </span>
            )}

            {/* Scale indicator dot for root notes */}
            {isRoot && !active && (
                <div style={{
                    position: 'absolute',
                    bottom: 6,
                    width: 4,
                    height: 4,
                    borderRadius: '50%',
                    background: 'var(--neon-cyan)',
                    opacity: 0.5
                }} />
            )}
        </button>
    );
};
