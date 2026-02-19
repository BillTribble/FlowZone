import React, { useState, useEffect, useRef } from 'react';

interface FaderProps {
    label?: string;
    value?: number; // 0.0 to 1.0 (gain)
    meterValue?: number; // 0.0 to 1.0 (VU)
    onChange?: (value: number) => void;
    height?: number | string;
    width?: number;
    color?: string;
}

export const Fader: React.FC<FaderProps> = ({
    label,
    value = 0.7,
    meterValue = 0,
    onChange,
    height = "100%",
    width = 32,
    color = 'var(--neon-cyan)'
}) => {
    const [internalValue, setInternalValue] = useState(value);
    const trackRef = useRef<HTMLDivElement>(null);
    const isDragging = useRef(false);

    useEffect(() => {
        setInternalValue(value);
    }, [value]);

    const handlePointerMove = (e: PointerEvent) => {
        if (!isDragging.current || !trackRef.current) return;
        const rect = trackRef.current.getBoundingClientRect();
        const relativeY = e.clientY - rect.top;
        const normalized = 1 - (relativeY / rect.height);
        const clamped = Math.max(0, Math.min(1, normalized));

        setInternalValue(clamped);
        onChange?.(clamped);
    };

    const handlePointerUp = () => {
        isDragging.current = false;
        window.removeEventListener('pointermove', handlePointerMove);
        window.removeEventListener('pointerup', handlePointerUp);
    };

    const handlePointerDown = (e: React.PointerEvent) => {
        isDragging.current = true;
        const target = trackRef.current;
        if (target && 'setPointerCapture' in target) {
            target.setPointerCapture(e.pointerId);
        }

        const rect = trackRef.current?.getBoundingClientRect();
        if (rect && rect.height > 0) {
            const relativeY = e.clientY - rect.top;
            const normalized = 1 - (relativeY / rect.height);
            const clamped = Math.max(0, Math.min(1, normalized));

            setInternalValue(clamped);
            onChange?.(clamped);
        }

        window.addEventListener('pointermove', handlePointerMove);
        window.addEventListener('pointerup', handlePointerUp);
    };

    const capHeight = 24;

    return (
        <div style={{
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            gap: 8,
            height: height === "100%" ? '100%' : height
        }}>
            <div
                ref={trackRef}
                onPointerDown={handlePointerDown}
                className="glass-panel"
                style={{
                    width,
                    flex: 1,
                    position: 'relative',
                    background: 'rgba(0,0,0,0.3)',
                    cursor: 'ns-resize',
                    touchAction: 'none',
                    border: '1px solid var(--glass-border)'
                }}
            >
                {/* Track Center Line */}
                <div style={{
                    position: 'absolute',
                    left: '50%',
                    top: 10,
                    bottom: 10,
                    width: 2,
                    background: 'rgba(255,255,255,0.05)',
                    transform: 'translateX(-50%)'
                }} />

                {/* VU Meter */}
                <div style={{
                    position: 'absolute',
                    bottom: 0,
                    left: 0,
                    right: 0,
                    height: `${meterValue * 100}%`,
                    background: `linear-gradient(to top, var(--neon-cyan) 0%, #fff 100%)`,
                    opacity: 0.2,
                    transition: 'height 0.05s linear',
                    filter: 'blur(4px)'
                }} />

                {/* Level Fill */}
                <div style={{
                    position: 'absolute',
                    bottom: 0,
                    left: '50%',
                    width: 2,
                    height: `${internalValue * 100}%`,
                    background: color,
                    transform: 'translateX(-50%)',
                    boxShadow: `0 0 10px ${color}`
                }} />

                {/* Fader Cap */}
                <div style={{
                    position: 'absolute',
                    bottom: `calc(${internalValue * 100}% - ${capHeight / 2}px)`,
                    left: -4,
                    right: -4,
                    height: capHeight,
                    background: '#1a1b1e',
                    border: `1px solid ${isDragging.current ? color : 'var(--glass-border)'}`,
                    borderRadius: 4,
                    boxShadow: isDragging.current ? `0 0 15px ${color}` : '0 4px 8px rgba(0,0,0,0.5)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    zIndex: 2,
                    transition: 'border 0.1s ease, box-shadow 0.1s ease'
                }}>
                    <div style={{ width: '40%', height: 2, background: isDragging.current ? color : '#666', borderRadius: 1 }} />
                </div>
            </div>

            {label && (
                <span style={{
                    fontSize: '10px',
                    color: 'var(--text-secondary)',
                    textTransform: 'uppercase',
                    fontWeight: 'bold',
                    letterSpacing: '0.05em'
                }}>
                    {label}
                </span>
            )}
        </div>
    );
};
