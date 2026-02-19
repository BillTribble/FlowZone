import React, { useState, useEffect, useRef } from 'react';

interface KnobProps {
    label: string;
    value?: number; // 0.0 to 1.0
    onChange?: (value: number) => void;
    size?: number;
    color?: string;
    min?: number;
    max?: number;
    step?: number;
}

export const Knob: React.FC<KnobProps> = ({
    label,
    value = 0,
    onChange,
    size = 60,
    color = '#00E5FF',
    min = 0,
    max = 1,
    step = 0.01
}) => {
    const [internalValue, setInternalValue] = useState(value);
    const isDragging = useRef(false);
    const startY = useRef(0);
    const startVal = useRef(0);

    useEffect(() => {
        setInternalValue(value);
    }, [value]);

    const handlePointerDown = (e: React.PointerEvent) => {
        e.preventDefault();
        e.currentTarget.setPointerCapture(e.pointerId);
        isDragging.current = true;
        startY.current = e.clientY;
        startVal.current = internalValue;
    };

    const handlePointerMove = (e: React.PointerEvent) => {
        if (!isDragging.current) return;
        e.preventDefault();

        const deltaY = startY.current - e.clientY;
        const range = max - min;
        // Sensitivity: full range in 200px
        const deltaVal = (deltaY / 200) * range;

        let newVal = startVal.current + deltaVal;
        newVal = Math.max(min, Math.min(max, newVal));

        // Snap to step
        if (step > 0) {
            newVal = Math.round(newVal / step) * step;
        }

        setInternalValue(newVal);
        if (onChange) onChange(newVal);
    };

    const handlePointerUp = (e: React.PointerEvent) => {
        isDragging.current = false;
        e.currentTarget.releasePointerCapture(e.pointerId);
    };

    // Calculate arc
    const radius = size * 0.4;
    const center = size / 2;
    const strokeWidth = size * 0.1;

    // 270 degree arc (-135 to +135)
    const startAngle = -135 * (Math.PI / 180);
    const endAngle = 135 * (Math.PI / 180);
    const angleRange = endAngle - startAngle;

    const normalizedValue = (internalValue - min) / (max - min);
    const currentAngle = startAngle + (normalizedValue * angleRange);

    // Helper for polar to cartesian
    const polarToCartesian = (centerX: number, centerY: number, radius: number, angleInRadians: number) => {
        return {
            x: centerX + (radius * Math.cos(angleInRadians - Math.PI / 2)),
            y: centerY + (radius * Math.sin(angleInRadians - Math.PI / 2))
        };
    };

    // SVG Arc path
    const createArc = (start: number, end: number) => {
        const startPt = polarToCartesian(center, center, radius, start);
        const endPt = polarToCartesian(center, center, radius, end);
        const largeArcFlag = end - start <= Math.PI ? "0" : "1";
        return [
            "M", startPt.x, startPt.y,
            "A", radius, radius, 0, largeArcFlag, 1, endPt.x, endPt.y
        ].join(" ");
    };

    return (
        <div style={{
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            gap: 4,
            userSelect: 'none',
            touchAction: 'none' // Prevent scrolling on touch
        }}>
            <div
                onPointerDown={handlePointerDown}
                onPointerMove={handlePointerMove}
                onPointerUp={handlePointerUp}
                onPointerLeave={handlePointerUp}
                style={{
                    width: size,
                    height: size,
                    position: 'relative',
                    cursor: 'ns-resize',
                    touchAction: 'none'
                }}
            >
                <svg width={size} height={size}>
                    {/* Background Track */}
                    <path
                        d={createArc(startAngle, endAngle)}
                        fill="none"
                        stroke="#333"
                        strokeWidth={strokeWidth}
                        strokeLinecap="round"
                    />
                    {/* Active Arc */}
                    <path
                        d={createArc(startAngle, currentAngle)}
                        fill="none"
                        stroke={color}
                        strokeWidth={strokeWidth}
                        strokeLinecap="round"
                    />
                </svg>
            </div>
            <span style={{ fontSize: '10px', color: '#888', textTransform: 'uppercase', letterSpacing: '0.5px' }}>
                {label}
            </span>
        </div>
    );
};
