import React from 'react';

interface OblongProps {
    label?: string;
    active?: boolean;
    color?: string;
    width?: number;
    height?: number;
    onClick?: () => void;
}

export const Oblong: React.FC<OblongProps> = ({
    label,
    active = false,
    color = '#00E5FF',
    width = 40,
    height = 12,
    onClick
}) => {
    return (
        <div
            onClick={onClick}
            style={{
                width,
                height,
                borderRadius: height / 2,
                backgroundColor: active ? color : '#333',
                border: `1px solid ${active ? color : '#444'}`,
                boxShadow: active ? `0 0 8px ${color}66` : 'none',
                cursor: onClick ? 'pointer' : 'default',
                transition: 'all 0.1s ease',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center'
            }}
        >
            {label && (
                <span style={{
                    fontSize: '8px',
                    color: active ? '#000' : '#888',
                    fontWeight: 'bold',
                    pointerEvents: 'none'
                }}>
                    {label}
                </span>
            )}
        </div>
    );
};
