import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { SlotIndicator } from './SlotIndicator';

describe('SlotIndicator', () => {
    it('renders with label and source color', () => {
        render(
            <SlotIndicator
                slotId={0}
                source="drums"
                label="DRUMS"
            />
        );
        expect(screen.getByText('DRUMS')).toBeDefined();
    });

    it('shows MUTE overlay when isMuted is true', () => {
        render(
            <SlotIndicator
                slotId={0}
                source="drums"
                isMuted={true}
            />
        );
        expect(screen.getByText('MUTE')).toBeDefined();
    });

    it('calls onClick when clicked', () => {
        const onClick = vi.fn();
        render(
            <SlotIndicator
                slotId={5}
                source="notes"
                onClick={onClick}
            />
        );
        fireEvent.click(screen.getByText('S6'));
        expect(onClick).toHaveBeenCalledWith(5);
    });

    it('renders progress bar when isLooping is true', () => {
        const { container } = render(
            <SlotIndicator
                slotId={0}
                source="bass"
                isLooping={true}
                progress={0.5}
            />
        );
        const progressFill = container.querySelector('div[style*="width: 50%"]');
        expect(progressFill).toBeDefined();
    });
});
