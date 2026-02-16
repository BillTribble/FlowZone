import { useState, useEffect } from 'react'
import { WebSocketClient } from './api/WebSocketClient'
import { AppState } from '../../shared/protocol/schema'
import { MainLayout } from './components/layout/MainLayout'
import { TabId } from './components/layout/Navigation'
import { JamManagerView } from './views/JamManagerView'
import { PlayView } from './views/PlayView'
import { MixerView, MixerControls } from './views/MixerView'
import { ModeView } from './views/ModeView'
import { AdjustView } from './views/AdjustView'

function App() {
    const [state, setState] = useState<AppState | null>(null)
    const [connected, setConnected] = useState(false)
    const [activeTab, setActiveTab] = useState<TabId>('jam-manager')
    const [selectedPreset, setSelectedPreset] = useState<string | null>(null)
    const [selectedCategory, setSelectedCategory] = useState<string>('drums')

    // Initialize client once
    const [wsClient] = useState(() => new WebSocketClient('ws://localhost:50001'))

    useEffect(() => {
        console.log('[App] Initializing WebSocket connection...');
        wsClient.connect((newState) => {
            console.log('[App] State update received:', newState);
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
        console.log('[App] Pad trigger:', { padId, val });
        wsClient.send({ cmd: "NOTE_ON", pad: padId, val });
    };

    const handleXYChange = (x: number, y: number) => {
        wsClient.send({ cmd: "XY_CHANGE", x, y });
    };

    const handleSelectPreset = (category: string, preset: string) => {
        console.log('[App] Selecting preset:', { category, preset });
        setSelectedPreset(preset);
        wsClient.send({ cmd: "SET_PRESET", category, preset });
        // Auto-start playing when preset is selected
        if (!isPlaying) {
            console.log('[App] Auto-starting playback');
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
        console.log('[App] Mode selected:', mode);
        console.log('[App] Current state.activeMode:', state?.activeMode);
        
        // Update local category state
        setSelectedCategory(mode);
        
        // Send mode change to engine
        wsClient.send({ cmd: "SET_MODE", category: mode });
        
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
        console.log('[App] Home / Jam Manager clicked');
        setActiveTab('jam-manager');
    };

    const handleCreateJam = () => {
        console.log('[App] Create new jam');
        // TODO: Send CREATE_JAM command to engine
        // For now, just switch to MODE view to start a new jam
        setActiveTab('mode');
    };

    const handleOpenJam = (jamId: string) => {
        console.log('[App] Opening jam:', jamId);
        // TODO: Send LOAD_JAM command to engine
        // For now, switch to PLAY view (jam is already playing)
        setActiveTab('play');
    };

    const handleLoadRiff = (riffId: string) => {
        console.log('[App] Loading riff:', riffId);
        wsClient.send({ cmd: "LOAD_RIFF", riffId });
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
                console.log('[App] Toggle play:', !isPlaying);
                setIsPlaying(!isPlaying);
                wsClient.send({ cmd: "TOGGLE_PLAY" });
            }}
            performanceMode={performanceMode}
            onPadTrigger={handlePadTrigger}
            onXYChange={handleXYChange}
            bottomContent={bottomContent}
            onHomeClick={handleHomeClick}
            riffHistory={state?.riffHistory || []}
            onLoadRiff={handleLoadRiff}
        >
            {activeTab === 'jam-manager' && <JamManagerView onCreateJam={handleCreateJam} onOpenJam={handleOpenJam} />}
            {activeTab === 'mode' && <ModeView onSelectMode={handleSelectMode} />}
            {activeTab === 'play' && <PlayView
                state={{...state!, activeMode: {...state?.activeMode, category: selectedCategory}}}
                onSelectPreset={handleSelectPreset}
                selectedPreset={selectedPreset}
            />}
            {activeTab === 'adjust' && <AdjustView onAdjustParam={handleAdjustParam} />}
            {activeTab === 'mixer' && <MixerView
                state={state}
                onToggleMetronome={handleToggleMetronome}
                onSlotVolumeChange={handleSlotVolumeChange}
                onSelectSlot={handleSelectSlot}
                onMoreSettings={() => {
                    console.log('[App] More settings clicked');
                    // TODO: Open settings modal/view
                    alert('Advanced mixer settings coming soon!');
                }}
            />}
        </MainLayout>
    )
}

export default App
