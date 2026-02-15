import { render, screen } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import { PadGrid } from './PadGrid';

describe('PadGrid', () => {
    it('renders 16 pads', () => {
        const { container } = render(
            <PadGrid
                baseNote={48}
                scale="major"
                activePads={new Set()}
                onPadDown={() => { }}
                onPadUp={() => { }}
            />
        );
        const buttons = container.querySelectorAll('button');
        expect(buttons.length).toBe(16);
    });

    it('maps pad 0 to base note (C3)', () => {
        render(
            <PadGrid
                baseNote={48}
                scale="major"
                activePads={new Set()}
                onPadDown={() => { }}
                onPadUp={() => { }}
            />
        );
        expect(screen.getByText('C3')).toBeDefined();
    });
});
