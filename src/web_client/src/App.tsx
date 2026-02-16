import { useState, useEffect } from 'react'
import { WebSocketClient } from './api/WebSocketClient'
import { AppState } from '../../shared/protocol/schema'
import { MainLayout } from './components/layout/MainLayout'
import { TabId } from './components/layout/Navigation'
import { PlayView } from './views/PlayView'
import { MixerView, MixerControls } from './views/MixerView'
import { ModeView } from './views/ModeView'
import { AdjustView } from './views/AdjustView'

function App() {
    const [state, setState] = useState<AppState | null>(null)
    const [connected, setConnected] = useState(false)
    const [activeTab, setActiveTab] = useState<TabId>('play')
    const [selectedPreset, setSelectedPreset] = useState<string | null>(null)

    // Initialize client once
    const [wsClient] = useState(() => new WebSocketClient('ws://localhost:50001'))

    useEffect(() => {
        wsClient.connect((newState) => {
            setState(newState)
            setConnected(true)
        })
        return () => {
            // wsClient.disconnect()
        }
    }, [wsClient])

    const [performanceMode, setPerformanceMode] = useState<'PADS' | 'XY'>('PADS');
    const [isPlaying, setIsPlaying] = useState(false);

    if (!state) {
        return (
            <div style={{
                height: '100vh',
                background: '#121212',
                color: '#fff',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontFamily: 'sans-serif'
            }}>
                <div style={{ textAlign: 'center' }}>
                    <h1>FlowZone</h1>
                    <p style={{ color: '#666' }}>Connecting to Engine...</p>
                </div>
            </div>
        )
    }

    // WebSocket Handlers
    const handlePadTrigger = (padId: number, val: number) => {
        wsClient.send({ cmd: "NOTE_ON", pad: padId, val });
    };

    const handleXYChange = (x: number, y: number) => {
        wsClient.send({ cmd: "XY_CHANGE", x, y });
    };

    const handleSelectPreset = (category: string, preset: string) => {
        setSelectedPreset(preset);
        wsClient.send({ cmd: "SET_PRESET", category, preset });
        // Auto-start playing when preset is selected
        if (!isPlaying) {
            setIsPlaying(true);
            wsClient.send({ cmd: "TOGGLE_PLAY" });
        }
    };

    const handleToggleMetronome = () => {
        wsClient.send({ cmd: "TOGGLE_METRONOME" });
    };

    const handleSlotVolumeChange = (slotId: number, volume: number) => {
        wsClient.send({ cmd: "SET_SLOT_VOLUME", slot: slotId, volume });
    };

    const handleSelectSlot = (slotId: number) => {
        wsClient.send({ cmd: "SELECT_SLOT", slot: slotId });
    };

    const handleSelectMode = (mode: string) => {
        console.log('Selected mode:', mode);
        if (mode === 'fx' || mode === 'ext_fx') {
            setPerformanceMode('XY');
            setActiveTab('play');
        } else {
            setPerformanceMode('PADS');
            setActiveTab('play');
        }
    };

    const handleAdjustParam = (id: string, value: number) => {
        // Find param index from ID string or just pass it through if possible
        const paramId = parseInt(id) || 0;
        wsClient.send({ cmd: "ADJUST_PARAM", param: paramId, value });
    };

    const handleHomeClick = () => {
        console.log('Home / Jam Manager clicked');
        // Future: Navigate to Jam Manager view
        alert('Jam Manager coming soon!');
    };

    // Determine bottom content (Performance Surface or Mixer Controls)
    let bottomContent: React.ReactNode | undefined = undefined;
    if (activeTab === 'mixer') {
        bottomContent = <MixerControls
            state={state}
            onSlotVolumeChange={handleSlotVolumeChange}
        />;
    }

    return (
        <MainLayout
            activeTab={activeTab}
            onTabChange={setActiveTab}
            isConnected={connected}
            bpm={state?.transport?.bpm ?? 120}
            isPlaying={isPlaying || state?.transport?.isPlaying || false}
            onTogglePlay={() => {
                setIsPlaying(!isPlaying);
                wsClient.send({ cmd: "TOGGLE_PLAY" });
            }}
            performanceMode={performanceMode}
            onPadTrigger={handlePadTrigger}
            onXYChange={handleXYChange}
            bottomContent={bottomContent}
            onHomeClick={handleHomeClick}
        >
            {activeTab === 'mode' && <ModeView onSelectMode={handleSelectMode} />}
            {activeTab === 'play' && <PlayView state={state} onSelectPreset={handleSelectPreset} selectedPreset={selectedPreset} />}
            {activeTab === 'adjust' && <AdjustView onAdjustParam={handleAdjustParam} />}
            {activeTab === 'mixer' && <MixerView
                state={state}
                onToggleMetronome={handleToggleMetronome}
                onSlotVolumeChange={handleSlotVolumeChange}
                onSelectSlot={handleSelectSlot}
            />}
        </MainLayout>
    )
}

export default App
