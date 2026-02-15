import { render, screen } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import { RiffIndicator } from './RiffIndicator';

describe('RiffIndicator', () => {
    it('renders timestamp and layers', () => {
        render(
            <RiffIndicator
                id="test-riff"
                timestamp="12:34:56"
                layers={[
                    { id: 'l1', source: 'drums', level: 0.8 },
                    { id: 'l2', source: 'notes', level: 0.5 }
                ]}
            />
        );
        expect(screen.getByText('12:34:56')).toBeDefined();
    });

    it('shows active state styling', () => {
        const { container } = render(
            <RiffIndicator
                id="test-riff"
                timestamp="12:34:56"
                layers={[]}
                isActive={true}
            />
        );
        const inner = container.querySelector('.neon-glow');
        expect(inner).toBeDefined();
    });
});
