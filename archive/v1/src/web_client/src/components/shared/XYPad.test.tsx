import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { XYPad } from './XYPad';

describe('XYPad', () => {
    it('renders with label', () => {
        render(<XYPad label="PITCH" onChange={() => { }} />);
        expect(screen.getByText('PITCH')).toBeDefined();
    });

    it('calls onChange when clicked', () => {
        const onChange = vi.fn();
        const { container } = render(<XYPad onChange={onChange} />);

        const track = container.querySelector('.glass-panel') as HTMLElement;

        // Mock getBoundingClientRect
        vi.spyOn(track, 'getBoundingClientRect').mockReturnValue({
            top: 0,
            left: 0,
            width: 100,
            height: 100,
            bottom: 100,
            right: 100,
            x: 0,
            y: 0,
            toJSON: () => { }
        } as DOMRect);

        fireEvent.pointerDown(track, { clientX: 25, clientY: 75, buttons: 1 });

        // normalized X=0.25, Y=1-(75/100)=0.25
        expect(onChange).toHaveBeenCalled();
        const callValue = onChange.mock.calls[0];
        expect(callValue[0]).toBeCloseTo(0.25);
        expect(callValue[1]).toBeCloseTo(0.25);
    });
});
