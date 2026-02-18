import React, { useRef, useEffect } from 'react';

interface WaveformCanvasProps {
    data: Float32Array | number[];
    color?: string;
    backgroundColor?: string;
    height?: number | string;
    width?: number | string;
    lineWidth?: number;
    opacity?: number;
}

export const WaveformCanvas: React.FC<WaveformCanvasProps> = ({
    data,
    color = 'var(--neon-cyan)',
    backgroundColor = 'rgba(0,0,0,0.1)',
    height = 100,
    width = '100%',
    lineWidth = 2,
    opacity = 1
}) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        if (data && data.length > 0) {
            const hasData = data.some(v => v > 0.001);
            if (hasData) {
                console.log('[WaveformCanvas] Rendering data with length:', data.length, 'max sample:', Math.max(...Array.from(data)));
            }
        }

        // Handle high DPI displays
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.scale(dpr, dpr);

        const w = rect.width;
        const h = rect.height;

        ctx.clearRect(0, 0, w, h);

        if (backgroundColor) {
            ctx.fillStyle = backgroundColor;
            ctx.fillRect(0, 0, w, h);
        }

        if (data.length === 0) return;

        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = lineWidth;
        ctx.lineJoin = 'round';
        ctx.globalAlpha = opacity;

        const step = w / data.length;
        const midY = h / 2;

        for (let i = 0; i < data.length; i++) {
            const x = i * step;
            const val = Math.max(0.01, data[i]); // Min visible height
            const amplitude = (val * (h / 2) * 0.9);

            // Draw top half
            ctx.moveTo(x, midY - amplitude);
            ctx.lineTo(x, midY + amplitude);
        }

        ctx.stroke();

        // Optional: add a subtle neon glow to the waveform
        ctx.shadowBlur = 5;
        ctx.shadowColor = color;
        ctx.stroke();

    }, [data, color, backgroundColor, lineWidth, opacity, width, height]);

    return (
        <div style={{
            width: width === '100%' ? '100%' : width,
            height: height,
            overflow: 'hidden',
            borderRadius: 4,
            position: 'relative'
        }}>
            <canvas
                ref={canvasRef}
                style={{
                    width: '100%',
                    height: '100%',
                    display: 'block'
                }}
            />
        </div>
    );
};
