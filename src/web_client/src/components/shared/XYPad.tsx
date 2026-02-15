import React, { useState, useRef, useEffect } from 'react';

interface XYPadProps {
    value?: { x: number, y: number };
    onChange: (x: number, y: number) => void;
    onEngage?: () => void;
    onDisengage?: () => void;
    label?: string;
}

export const XYPad: React.FC<XYPadProps> = ({
    value = { x: 0.5, y: 0.5 },
    onChange,
    onEngage,
    onDisengage,
    label = "XY SURFACE"
}) => {
    const [localPos, setLocalPos] = useState(value);
    const padRef = useRef<HTMLDivElement>(null);
    const isDragging = useRef(false);

    useEffect(() => {
        setLocalPos(value);
    }, [value]);

    const handlePointer = (e: React.PointerEvent) => {
        if (!padRef.current) return;
        const rect = padRef.current.getBoundingClientRect();

        const x = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
        const y = Math.max(0, Math.min(1, 1 - (e.clientY - rect.top) / rect.height));

        const newVal = { x, y };
        setLocalPos(newVal);
        onChange(x, y);
    };

    const handlePointerDown = (e: React.PointerEvent) => {
        isDragging.current = true;
        if (padRef.current && 'setPointerCapture' in padRef.current) {
            padRef.current.setPointerCapture(e.pointerId);
        }
        onEngage?.();
        handlePointer(e);
    };

    const handlePointerMove = (e: React.PointerEvent) => {
        if (isDragging.current) {
            handlePointer(e);
        }
    };

    const handlePointerUp = () => {
        isDragging.current = false;
        onDisengage?.();
    };

    return (
        <div
            ref={padRef}
            className="glass-panel interactive-element"
            style={{
                height: '100%',
                width: '100%',
                position: 'relative',
                overflow: 'hidden',
                cursor: 'crosshair',
                touchAction: 'none'
            }}
            onPointerDown={handlePointerDown}
            onPointerMove={handlePointerMove}
            onPointerUp={handlePointerUp}
            onPointerCancel={handlePointerUp}
        >
            {/* Background Grid */}
            <div style={{
                position: 'absolute',
                inset: 0,
                opacity: 0.1,
                background: `
                    linear-gradient(to right, var(--text-secondary) 1px, transparent 1px),
                    linear-gradient(to bottom, var(--text-secondary) 1px, transparent 1px)
                `,
                backgroundSize: '10% 10%'
            }} />

            {/* Label */}
            <div style={{
                position: 'absolute',
                inset: 0,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                pointerEvents: 'none'
            }}>
                <span style={{
                    color: 'var(--text-secondary)',
                    fontSize: '24px',
                    fontWeight: 900,
                    letterSpacing: '0.2em',
                    opacity: 0.1
                }}>
                    {label}
                </span>
            </div>

            {/* Crosshair lines */}
            <div style={{
                position: 'absolute',
                left: `${localPos.x * 100}%`,
                top: 0,
                bottom: 0,
                width: '1px',
                background: 'var(--neon-cyan)',
                opacity: isDragging.current ? 0.4 : 0.2,
                pointerEvents: 'none'
            }} />
            <div style={{
                position: 'absolute',
                top: `${(1 - localPos.y) * 100}%`,
                left: 0,
                right: 0,
                height: '1px',
                background: 'var(--neon-cyan)',
                opacity: isDragging.current ? 0.4 : 0.2,
                pointerEvents: 'none'
            }} />

            {/* Interactive Puck */}
            <div style={{
                position: 'absolute',
                left: `${localPos.x * 100}%`,
                top: `${(1 - localPos.y) * 100}%`,
                width: 32,
                height: 32,
                borderRadius: '50%',
                background: isDragging.current ? 'var(--neon-cyan)' : 'transparent',
                border: '2px solid var(--neon-cyan)',
                transform: 'translate(-50%, -50%)',
                boxShadow: `0 0 20px 2px var(--neon-cyan)`,
                transition: 'background 0.1s ease, width 0.1s ease, height 0.1s ease',
                pointerEvents: 'none'
            }}>
                {/* Secondary ring */}
                <div style={{
                    position: 'absolute',
                    inset: -4,
                    border: '1px solid var(--neon-cyan)',
                    borderRadius: '50%',
                    opacity: 0.3
                }} />
            </div>
        </div>
    );
};
