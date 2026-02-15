import { useState, useEffect } from 'react'
import { WebSocketClient } from './api/WebSocketClient'
import { AppState } from '../../shared/protocol/schema'
import { MainLayout } from './components/layout/MainLayout'
import { TabId } from './components/layout/Navigation'
import { PlayView } from './views/PlayView'
import { MixerView } from './views/MixerView'
import { ModeView } from './views/ModeView'
import { AdjustView } from './views/AdjustView'

function App() {
    const [state, setState] = useState<AppState | null>(null)
    const [connected, setConnected] = useState(false)
    const [activeTab, setActiveTab] = useState<TabId>('play')

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

    return (
        <MainLayout
            activeTab={activeTab}
            onTabChange={setActiveTab}
            isConnected={connected}
            bpm={120} // TODO: Get from store
            isPlaying={false} // TODO: Get from store
            onTogglePlay={() => { }}
            performanceMode={performanceMode}
            onPadTrigger={handlePadTrigger}
            onXYChange={handleXYChange}
        >
            {activeTab === 'mode' && <ModeView onSelectMode={(mode) => {
                console.log('Selected mode:', mode);
                if (mode === 'fx' || mode === 'ext_fx') {
                    setPerformanceMode('XY');
                    setActiveTab('play');
                    // User said: "In FX mode, xy surface should be visible... Top half contains controls for play / adjust"
                    // Let's default to 'adjust' for FX, or maybe just 'play' is fine effectively.
                    // Let's keep it simple: FX -> XY Surface.
                } else {
                    setPerformanceMode('PADS');
                    setActiveTab('play');
                }
            }} />}
            {activeTab === 'play' && <PlayView state={state} />}
            {activeTab === 'adjust' && <AdjustView state={state} />}
            {activeTab === 'mixer' && <MixerView state={state} onToggleMetronome={() => { }} />}
        </MainLayout>
    )
}

export default App
