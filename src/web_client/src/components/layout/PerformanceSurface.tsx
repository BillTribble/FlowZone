import React from 'react';
import { PadGrid } from '../shared/PadGrid';
import { XYPad } from '../shared/XYPad';

interface PerformanceSurfaceProps {
    mode: 'PADS' | 'XY';
    onPadTrigger: (padId: number, velocity: number) => void;
    onPadRelease: (padId: number) => void;
    onXYChange: (x: number, y: number) => void;
    activeCategory: string;
}

export const PerformanceSurface: React.FC<PerformanceSurfaceProps> = ({ mode, onPadTrigger, onPadRelease, onXYChange, activeCategory }) => {
    if (mode === 'XY') {
        return (
            <XYPad
                onChange={onXYChange}
                onEngage={() => {
                    // console.log('XY Engaged');
                    // In a real app, we might send FX_ENGAGE here
                }}
                onDisengage={() => {
                    // console.log('XY Disengaged');
                    // In a real app, we might send FX_DISENGAGE here
                }}
            />
        );
    }

    // Default: PADS (4x4 Grid)
    // In a real app, these would come from state or context
    const [activePads, setActivePads] = React.useState<Set<number>>(new Set());

    const handlePadDown = (midi: number, padId: number) => {
        console.log('[PerformanceSurface] Pad down - midi:', midi, 'padId:', padId);
        setActivePads(prev => new Set(prev).add(padId));
        onPadTrigger(midi, 1.0); // Using midi instead of padId as 'note'
    };

    const handlePadUp = (midi: number, padId: number) => {
        setActivePads(prev => {
            const next = new Set(prev);
            next.delete(padId);
            return next;
        });
        // Send NOTE_OFF for synths (not needed for drums as they're one-shot)
        if (activeCategory !== 'drums') {
            onPadRelease(midi);
        }
    };

    return (
        <div style={{
            height: '100%',
            width: '100%',
            display: 'flex',
            boxSizing: 'border-box',
            overflow: 'hidden'
        }}>
            <PadGrid
                baseNote={activeCategory === 'drums' ? 36 : 48}
                scale="major"
                activePads={activePads}
                onPadDown={handlePadDown}
                onPadUp={handlePadUp}
            />
        </div>
    );
};
