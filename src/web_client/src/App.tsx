import { useState, useEffect } from 'react'
import { mockStateProvider } from './api/MockStateProvider'
import { AppState } from '../../shared/protocol/schema'

function App() {
    const [state, setState] = useState<AppState>(mockStateProvider.getState())

    useEffect(() => {
        const unsubscribe = mockStateProvider.subscribe(setState)
        return unsubscribe
    }, [])

    return (
        <div style={{ padding: 20, font: 'sans-serif', color: 'white', background: '#222' }}>
            <h1>FlowZone</h1>
            <p>Web Client v0.0.1</p>

            <div style={{ border: '1px solid #444', padding: 10, margin: '10px 0' }}>
                <div style={{ display: 'flex', gap: '10px', alignItems: 'center' }}>
                    <h2>Transport</h2>
                    <button onClick={() => mockStateProvider.togglePlay()}>
                        {state.transport.isPlaying ? 'PAUSE' : 'PLAY'}
                    </button>
                </div>
                <p>BPM: {state.transport.bpm}</p>
                <p>Phase: {state.transport.barPhase.toFixed(2)}</p>
                <p>Session: {state.session.name}</p>
            </div>

            <div style={{ display: 'grid', gap: 10 }}>
                {state.slots.map(slot => (
                    <div key={slot.id} style={{ background: '#333', padding: 10 }}>
                        <h3>{slot.name}</h3>
                        <p>State: {slot.state}</p>
                        <p>Vol: {slot.volume}</p>
                    </div>
                ))}
            </div>
        </div>
    )
}

export default App
