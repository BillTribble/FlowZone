import { render } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import { WaveformCanvas } from './WaveformCanvas';

describe('WaveformCanvas', () => {
    it('renders a canvas element', () => {
        const { container } = render(
            <WaveformCanvas
                data={new Float32Array(10).fill(0)}
                height={50}
            />
        );
        const canvas = container.querySelector('canvas');
        expect(canvas).toBeDefined();
    });
});
