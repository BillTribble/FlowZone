import React, { useState, useEffect, useRef } from 'react';

interface FaderProps {
    label?: string;
    value?: number; // 0.0 to 1.0 (gain)
    meterValue?: number; // 0.0 to 1.0 (VU)
    onChange?: (value: number) => void;
    height?: number;
    width?: number;
    color?: string;
}

export const Fader: React.FC<FaderProps> = ({
    label,
    value = 0.7,
    meterValue = 0,
    onChange,
    height = 200,
    width = 40,
    color = '#00E5FF'
}) => {
    const [internalValue, setInternalValue] = useState(value);
    const trackRef = useRef<HTMLDivElement>(null);
    const isDragging = useRef(false);

    useEffect(() => {
        setInternalValue(value);
    }, [value]);

    const handleMouseDown = (e: React.MouseEvent) => {
        isDragging.current = true;
        updateValueFromMouse(e.clientY);
        window.addEventListener('mousemove', handleMouseMove);
        window.addEventListener('mouseup', handleMouseUp);
    };

    const handleMouseMove = (e: MouseEvent) => {
        if (!isDragging.current) return;
        updateValueFromMouse(e.clientY);
    };

    const handleMouseUp = () => {
        isDragging.current = false;
        window.removeEventListener('mousemove', handleMouseMove);
        window.removeEventListener('mouseup', handleMouseUp);
    };

    const updateValueFromMouse = (clientY: number) => {
        if (!trackRef.current) return;
        const rect = trackRef.current.getBoundingClientRect();
        const relativeY = clientY - rect.top;
        // Invert Y because fader 1.0 is at top
        const normalized = 1 - (relativeY / rect.height);
        const clamped = Math.max(0, Math.min(1, normalized));

        setInternalValue(clamped);
        if (onChange) onChange(clamped);
    };

    // Cap Size
    const capHeight = 30;

    return (
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 8 }}>
            <div
                ref={trackRef}
                onMouseDown={handleMouseDown}
                style={{
                    width,
                    height,
                    position: 'relative',
                    background: '#222',
                    borderRadius: 4,
                    overflow: 'hidden',
                    cursor: 'ns-resize',
                    border: '1px solid #333'
                }}
            >
                {/* VU Meter Background */}
                <div style={{
                    position: 'absolute',
                    bottom: 0,
                    left: 0,
                    right: 0,
                    height: `${meterValue * 100}%`,
                    background: `linear-gradient(to top, #0f0 0%, #ff0 80%, #f00 100%)`,
                    opacity: 0.3,
                    transition: 'height 0.05s linear'
                }} />

                {/* Fader Cap */}
                <div style={{
                    position: 'absolute',
                    bottom: `calc(${internalValue * 100}% - ${capHeight / 2}px)`,
                    left: 2,
                    right: 2,
                    height: capHeight,
                    background: '#444',
                    border: `1px solid ${color}`,
                    borderRadius: 2,
                    boxShadow: '0 2px 4px rgba(0,0,0,0.5)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                }}>
                    <div style={{ width: '60%', height: 2, background: color }} />
                </div>
            </div>
            {label && (
                <span style={{ fontSize: '10px', color: '#888', textTransform: 'uppercase' }}>
                    {label}
                </span>
            )}
        </div>
    );
};
