import React from 'react';
import { Pad } from '../shared/Pad';

interface PerformanceSurfaceProps {
    mode: 'PADS' | 'XY';
    onPadTrigger: (padId: number, velocity: number) => void;
    onXYChange: (x: number, y: number) => void;
}

export const PerformanceSurface: React.FC<PerformanceSurfaceProps> = ({ mode, onPadTrigger, onXYChange }) => {
    // XY Cursor Position State
    const [xyPos, setXyPos] = React.useState({ x: 0.5, y: 0.5 });

    // Update local state when prop changes if needed, but for now we drive local visual from interaction
    // Ideally we should receive the current XY from state to perfectly sync, but prediction is fine for now

    if (mode === 'XY') {
        const handlePointer = (e: React.PointerEvent) => {
            if (e.buttons === 1) {
                const rect = e.currentTarget.getBoundingClientRect();
                const x = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
                const y = Math.max(0, Math.min(1, 1 - (e.clientY - rect.top) / rect.height)); // 0 at bottom
                setXyPos({ x, y });
                onXYChange(x, y);
                e.currentTarget.setPointerCapture(e.pointerId);
            }
        };

        return (
            <div className="h-full w-full bg-gray-900 rounded-lg border-2 border-gray-700 relative overflow-hidden"
                style={{ touchAction: 'none' }}
                onPointerDown={handlePointer}
                onPointerMove={handlePointer}
            >
                <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
                    <span className="text-gray-500 font-bold text-xl opacity-20">XY SURFACE</span>
                </div>

                {/* Visual Puck */}
                <div style={{
                    position: 'absolute',
                    left: `${xyPos.x * 100}%`,
                    bottom: `${xyPos.y * 100}%`,
                    width: 24,
                    height: 24,
                    borderRadius: '50%',
                    background: '#00E5FF',
                    transform: 'translate(-50%, 50%)',
                    boxShadow: '0 0 10px #00E5FF',
                    pointerEvents: 'none'
                }} />
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
