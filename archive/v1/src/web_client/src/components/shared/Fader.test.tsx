import { render, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { Fader } from './Fader';

describe('Fader', () => {
    it('calls onChange when dragged', () => {
        const onChange = vi.fn();
        const { container } = render(<Fader onChange={onChange} height={100} />);

        const track = container.querySelector('.glass-panel') as HTMLElement;

        // Mock setPointerCapture
        track.setPointerCapture = vi.fn();
        track.releasePointerCapture = vi.fn();

        // Mock getBoundingClientRect
        vi.spyOn(track, 'getBoundingClientRect').mockReturnValue({
            top: 0,
            left: 0,
            width: 20,
            height: 100,
            bottom: 100,
            right: 20,
            x: 0,
            y: 0,
            toJSON: () => { }
        } as DOMRect);

        // Click at 75% down (relativeY = 75), which is 25% value
        // Use fireEvent with specific event properties that React reads
        fireEvent.pointerDown(track, {
            clientY: 75,
            pointerId: 1,
            buttons: 1
        });

        expect(onChange).toHaveBeenCalled();
        const callValue = onChange.mock.calls[0][0];
        expect(callValue).toBeCloseTo(0.25);
    });

    it('renders label in uppercase style', () => {
        const { getByText } = render(<Fader label="volume" />);
        expect(getByText(/VOLUME/i)).toBeDefined();
    });
});
