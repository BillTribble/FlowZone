import React from 'react';
import { Header } from './Header';
import { Navigation, TabId } from './Navigation';
import { PerformanceSurface } from './PerformanceSurface';
import { WaveformCanvas } from '../shared/WaveformCanvas';
import { RiffIndicator } from '../shared/RiffIndicator';

interface MainLayoutProps {
    activeTab: TabId;
    onTabChange: (tab: TabId) => void;
    isConnected: boolean;
    bpm: number;
    isPlaying: boolean;
    onTogglePlay: () => void;
    onPanic?: () => void;
    children: React.ReactNode;
    performanceMode: 'PADS' | 'XY';
    onPadTrigger: (padId: number, val: number) => void;
    onPadRelease: (padId: number) => void;
    onXYChange: (x: number, y: number) => void;
    activeCategory: string;
    activePads?: Set<number>;
    onHomeClick?: () => void;
    riffHistory?: any[];
    onLoadRiff?: (riffId: string) => void;
    waveformData?: Float32Array;
    looperInputLevel?: number;
}



export const MainLayout: React.FC<MainLayoutProps & { bottomContent?: React.ReactNode }> = ({
    activeTab,
    onTabChange,
    isConnected,
    bpm,
    isPlaying,
    onTogglePlay,
    onPanic,
    children,
    performanceMode,
    onPadTrigger,
    onPadRelease,
    onXYChange,
    activeCategory,
    activePads,
    bottomContent,
    onHomeClick,
    riffHistory = [],
    onLoadRiff,
    waveformData,
    looperInputLevel = 0
}) => {
    // Generate fallback waveform data if not provided
    const displayWaveform = waveformData || new Float32Array(256).fill(0);
    return (
        <div style={{
            display: 'flex',
            flexDirection: 'column',
            height: '100vh',
            width: '100vw',
            background: '#121212',
            color: '#fff',
            fontFamily: 'system-ui, -apple-system, sans-serif',
            overflow: 'hidden',
            touchAction: 'none' // Global touch action to help with preventDefault
        }}>
            {/* 1. Header */}
            <Header
                sessionName="FlowZone Session"
                bpm={bpm}
                isPlaying={isPlaying}
                connected={isConnected}
                onTogglePlay={onTogglePlay}
                onPanic={onPanic}
                onHomeClick={onHomeClick}
                looperInputLevel={looperInputLevel}
            />

            {/* 2. Top Half: View Content + Visualization */}
            <div style={{
                flex: 1,
                display: 'flex',
                flexDirection: 'column',
                overflow: 'hidden',
                position: 'relative',
                borderBottom: '1px solid var(--glass-border)',
                background: 'var(--bg-dark)'
            }}>
                {/* View Content */}
                <div style={{ flex: 1, overflowY: 'auto' }}>
                    {children}
                </div>

                {/* Riff History (Horizontal) */}
                <div style={{
                    height: 64,
                    padding: '8px 16px',
                    display: 'flex',
                    gap: 12,
                    overflowX: 'auto',
                    borderTop: '1px solid var(--glass-border)',
                    background: 'rgba(0,0,0,0.2)',
                    alignItems: 'center'
                }}>
                    {riffHistory.length > 0 ? (
                        riffHistory.map((riff, index) => (
                            <RiffIndicator
                                key={riff.id}
                                id={riff.id}
                                timestamp={new Date(riff.timestamp).toLocaleTimeString()}
                                layers={riff.colors?.map((color: string, i: number) => ({
                                    id: `layer-${i}`,
                                    source: 'notes',
                                    level: 0.8
                                })) || []}
                                isActive={index === 0}
                                onClick={onLoadRiff}
                            />
                        ))
                    ) : (
                        <div style={{
                            fontSize: 11,
                            color: 'var(--text-secondary)',
                            padding: '0 8px',
                            fontStyle: 'italic'
                        }}>
                            No riff history yet - start playing to create riffs
                        </div>
                    )}
                </div>

                {/* Waveform Area (Bottom of Top Half) */}
                <div style={{
                    height: 60,
                    background: 'rgba(0,0,0,0.3)',
                    borderTop: '1px solid var(--glass-border)'
                }}>
                    <WaveformCanvas
                        data={displayWaveform}
                        height={60}
                        color="var(--neon-cyan)"
                        opacity={0.6}
                    />
                </div>
            </div>

            {/* 3. Middle: Navigation Bar */}
            <div style={{ flex: '0 0 auto', zIndex: 10 }}>
                <Navigation activeTab={activeTab} onTabChange={onTabChange} />
            </div>

            {/* 4. Bottom Half: Performance Surface (Pads/XY) OR Component Controls */}
            <div style={{
                flex: 1,
                padding: 12,
                background: '#1a1a1a',
                display: 'flex',
                flexDirection: 'column',
                minHeight: 0,
                overflow: 'hidden'
            }}>
                {bottomContent ? (
                    bottomContent
                ) : (
                    <PerformanceSurface
                        mode={performanceMode}
                        onPadTrigger={onPadTrigger}
                        onPadRelease={onPadRelease}
                        onXYChange={onXYChange}
                        activeCategory={activeCategory}
                        externalActivePads={activePads}
                    />
                )}
            </div>
        </div>
    );
};
