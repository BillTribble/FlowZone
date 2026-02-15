import React from 'react';

interface PadProps {
    label?: string;
    active?: boolean;
    color?: string;
    onClick?: () => void;
    onMouseDown?: () => void;
    onMouseUp?: () => void;
    icon?: React.ReactNode;
}

export const Pad: React.FC<PadProps> = ({
    label,
    active = false,
    color = '#00E5FF',
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
            style={{
                width: '100%',
                aspectRatio: '1/1',
                borderRadius: 8,
                backgroundColor: active ? color : '#333',
                border: 'none',
                boxShadow: active ? `0 0 15px ${color}` : 'inset 0 0 10px rgba(0,0,0,0.5)',
                cursor: 'pointer',
                transition: 'all 0.05s ease',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                gap: 8,
                position: 'relative',
                overflow: 'hidden'
            }}
        >
            {/* Gloss overlay */}
            <div style={{
                position: 'absolute',
                top: 0,
                left: 0,
                right: 0,
                height: '40%',
                background: 'linear-gradient(to bottom, rgba(255,255,255,0.1), transparent)',
                pointerEvents: 'none'
            }} />

            {icon && (
                <div style={{ color: active ? '#000' : '#888', transform: active ? 'scale(0.95)' : 'scale(1)' }}>
                    {icon}
                </div>
            )}

            {label && (
                <span style={{
                    fontSize: '12px',
                    color: active ? '#000' : '#888',
                    fontWeight: 'bold',
                    pointerEvents: 'none'
                }}>
                    {label}
                </span>
            )}
        </button>
    );
};
