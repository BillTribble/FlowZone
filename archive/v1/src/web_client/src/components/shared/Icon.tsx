import React from 'react';

// Simple Icon wrapper. In a real app we might use Lucide or FontAwesome.
// Here we can use it to wrap simplistic SVG paths.

interface IconProps {
    name: string; // 'play' | 'pause' | 'settings' | 'menu' | etc
    size?: number;
    color?: string;
}

export const Icon: React.FC<IconProps> = ({ name, size = 24, color = 'currentColor' }) => {
    let path = "";

    switch (name) {
        case 'play':
            path = "M5 3l14 9-14 9V3z"; // Play triangle
            break;
        case 'pause':
            path = "M6 19h4V5H6v14zm8-14v14h4V5h-4z"; // Pause bars
            break;
        case 'settings':
            path = "M19.14,12.94c0.04-0.3,0.06-0.61,0.06-0.94c0-0.32-0.02-0.64-0.07-0.94l2.03-1.58c0.18-0.14,0.23-0.41,0.12-0.61 l-1.92-3.32c-0.12-0.22-0.37-0.29-0.59-0.22l-2.39,0.96c-0.5-0.38-1.03-0.7-1.62-0.94L14.4,2.81c-0.04-0.24-0.24-0.41-0.48-0.41 h-3.84c-0.24,0-0.43,0.17-0.47,0.41L9.25,5.35C8.66,5.59,8.12,5.92,7.63,6.29L5.24,5.33c-0.22-0.08-0.47,0-0.59,0.22L2.74,8.87 C2.62,9.08,2.66,9.34,2.86,9.48l2.03,1.58C4.84,11.36,4.8,11.69,4.8,12s0.02,0.64,0.07,0.94l-2.03,1.58 c-0.18,0.14-0.23,0.41-0.12,0.61l1.92,3.32c0.12,0.22,0.37,0.29,0.59,0.22l2.39-0.96c0.5,0.38,1.03,0.7,1.62,0.94l0.36,2.54 c0.05,0.24,0.24,0.41,0.48,0.41h3.84c0.24,0,0.44-0.17,0.47-0.41l0.36-2.54c0.59-0.24,1.13-0.56,1.62-0.94l2.39,0.96 c0.22,0.08,0.47,0,0.59-0.22l1.92-3.32c0.12-0.22,0.07-0.47-0.12-0.61L19.14,12.94z M12,15.6c-1.98,0-3.6-1.62-3.6-3.6 s1.62-3.6,3.6-3.6s3.6,1.62,3.6,3.6S13.98,15.6,12,15.6z";
            break;
        case 'menu':
            path = "M3 18h18v-2H3v2zm0-5h18v-2H3v2zm0-7v2h18V6H3z";
            break;
        case 'grid':
            path = "M4 4h4v4H4zm6 0h4v4h-4zm6 0h4v4h-4zM4 10h4v4H4zm6 0h4v4h-4zm6 0h4v4h-4zM4 16h4v4H4zm6 0h4v4h-4zm6 0h4v4h-4z";
            break;
        case 'wave':
            path = "M2 12c2.5 0 2.5 5 5 5s2.5-5 5-5 2.5 5 5 5 2.5-5 5-5"; // Simple wave approximation
            // Using a nicer path for wave
            return (
                <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M2 12s3-7 10-7 10 7 10 7-3 7-10 7-10-7-10-7Z" />
                    <circle cx="12" cy="12" r="3" />
                </svg>
            )
        case 'sliders':
            // Custom sliders icon
            return (
                <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <line x1="4" x2="4" y1="21" y2="14" />
                    <line x1="4" x2="4" y1="10" y2="3" />
                    <line x1="12" x2="12" y1="21" y2="12" />
                    <line x1="12" x2="12" y1="8" y2="3" />
                    <line x1="20" x2="20" y1="21" y2="16" />
                    <line x1="20" x2="20" y1="12" y2="3" />
                    <line x1="1" x2="7" y1="14" y2="14" />
                    <line x1="9" x2="15" y1="8" y2="8" />
                    <line x1="17" x2="23" y1="16" y2="16" />
                </svg>
            )
        default:
            // Circle fallback
            return <div style={{ width: size, height: size, background: color, borderRadius: '50%' }} />
    }

    return (
        <svg width={size} height={size} viewBox="0 0 24 24" fill={color}>
            <path d={path} />
        </svg>
    );
};
