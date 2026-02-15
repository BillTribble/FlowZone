import React from 'react';
import { Pad } from '../shared/Pad';

interface PerformanceSurfaceProps {
    mode: 'PADS' | 'XY';
    onPadTrigger: (padId: number, velocity: number) => void;
    onXYChange: (x: number, y: number) => void;
}

export const PerformanceSurface: React.FC<PerformanceSurfaceProps> = ({ mode, onPadTrigger, onXYChange }) => {
    if (mode === 'XY') {
        return (
            <div className="h-full w-full bg-gray-900 rounded-lg border-2 border-gray-700 relative overflow-hidden touch-none"
                onPointerMove={(e) => {
                    if (e.buttons === 1) {
                        const rect = e.currentTarget.getBoundingClientRect();
                        const x = (e.clientX - rect.left) / rect.width;
                        const y = 1 - (e.clientY - rect.top) / rect.height; // 0 at bottom
                        onXYChange(Math.max(0, Math.min(1, x)), Math.max(0, Math.min(1, y)));
                    }
                }}
            >
                <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
                    <span className="text-gray-500 font-bold text-xl">XY SURFACE</span>
                </div>
                {/* Visual crosshair could be added here */}
            </div>
        );
    }

    // Default: PADS (4x4 Grid)
    const pads = Array.from({ length: 16 }, (_, i) => i);

    return (
        <div style={{
            height: '100%',
            width: '100%',
            display: 'grid',
            gridTemplateColumns: 'repeat(4, 1fr)',
            gridTemplateRows: 'repeat(4, 1fr)', // Ensure 4 rows for 16 pads
            gap: 8,
            padding: 8
        }}>
            {pads.map(padId => (
                <Pad
                    key={padId}
                    label={`P${padId + 1}`}
                    color="#3b82f6" // Default blue
                    active={false}
                    onClick={() => onPadTrigger(padId, 1.0)}
                />
            ))}
        </div>
    );
};
