import React from 'react';
import { Header } from './Header';
import { Navigation, TabId } from './Navigation';
import { PerformanceSurface } from './PerformanceSurface';

interface MainLayoutProps {
    activeTab: TabId;
    onTabChange: (tab: TabId) => void;
    isConnected: boolean;
    bpm: number;
    isPlaying: boolean;
    onTogglePlay: () => void;
    children: React.ReactNode;
    performanceMode: 'PADS' | 'XY';
    onPadTrigger: (padId: number, val: number) => void;
    onXYChange: (x: number, y: number) => void;
}

export const MainLayout: React.FC<MainLayoutProps & { bottomContent?: React.ReactNode }> = ({
    activeTab,
    onTabChange,
    isConnected,
    bpm,
    isPlaying,
    onTogglePlay,
    children,
    performanceMode,
    onPadTrigger,
    onXYChange,
    bottomContent
}) => {
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
            />

            {/* 2. Top Half: View Content (Controls/Settings) */}
            <div style={{
                flex: 1,
                overflow: 'hidden',
                position: 'relative',
                borderBottom: '1px solid #333',
                background: '#121212'
            }}>
                {children}
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
                        onXYChange={onXYChange}
                    />
                )}
            </div>
        </div>
    );
};
