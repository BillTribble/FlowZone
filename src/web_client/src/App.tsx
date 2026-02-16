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
import { SettingsPanel } from './components/settings/SettingsPanel'
import { Knob } from './components/shared/Knob'
import { Fader } from './components/shared/Fader'

function App() {
    const [state, setState] = useState<AppState | null>(null)
    const [connected, setConnected] = useState(false)
    const [showJamManager, setShowJamManager] = useState(true) // Start at home screen
    const [showSettings, setShowSettings] = useState(false)
    const [activeTab, setActiveTab] = useState<TabId>('mode')
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

    // Track active pads for visual feedback
    const [activePads, setActivePads] = useState<Set<number>>(new Set());

    // Keyboard controls for testing - matches PadGrid scale-aware logic
    useEffect(() => {
        const keyToPadIndex: { [key: string]: number } = {
            'a': 0, 's': 1, 'd': 2, 'f': 3,
            'g': 4, 'h': 5, 'j': 6, 'k': 7,
            'l': 8, ';': 9, ':': 10, '"': 11
        };

        const SCALE_INTERVALS: { [key: string]: number[] } = {
            major: [0, 2, 4, 5, 7, 9, 11]
        };

        const getMidiNote = (padId: number, baseNote: number) => {
            if (selectedCategory === 'drums') {
                // Drums: direct mapping 36-51
                return baseNote + padId;
            } else {
                // Notes/Bass: scale-aware mapping
                const intervals = SCALE_INTERVALS.major;
                const numSteps = intervals.length;
                const octave = Math.floor(padId / numSteps);
                const degree = padId % numSteps;
                return baseNote + (octave * 12) + intervals[degree];
            }
        };

        const activeKeys = new Set<string>();

        const handleKeyDown = (e: KeyboardEvent) => {
            const key = e.key.toLowerCase();
            
            // Prevent repeat events
            if (activeKeys.has(key)) return;
            activeKeys.add(key);

            // Pad controls (ASDFGHJKL;:")
            if (key in keyToPadIndex) {
                e.preventDefault(); // Prevent stuck button sound
                const padIndex = keyToPadIndex[key];
                const baseNote = selectedCategory === 'drums' ? 36 : 48;
                const midiNote = getMidiNote(padIndex, baseNote);
                
                // Update visual state
                setActivePads(prev => new Set(prev).add(padIndex));
                
                handlePadTrigger(midiNote, 1.0);
                return;
            }

            // Loop length controls (1, 2, 3, 4)
            const loopLengths: { [key: string]: number } = {
                '1': 1, '2': 2, '3': 4, '4': 8
            };
            if (key in loopLengths) {
                e.preventDefault(); // Prevent stuck button sound
                console.log('[App] Keyboard loop trigger:', loopLengths[key], 'bars');
                wsClient.send({ cmd: "SET_LOOP_LENGTH", bars: loopLengths[key] });
            }
        };

        const handleKeyUp = (e: KeyboardEvent) => {
            const key = e.key.toLowerCase();
            activeKeys.delete(key);

            // Send NOTE_OFF for pads (not needed for drums, but needed for synths)
            if (key in keyToPadIndex) {
                const padIndex = keyToPadIndex[key];
                
                // Update visual state
                setActivePads(prev => {
                    const next = new Set(prev);
                    next.delete(padIndex);
                    return next;
                });
                
                if (selectedCategory !== 'drums') {
                    const baseNote = 48;
                    const midiNote = getMidiNote(padIndex, baseNote);
                    handlePadRelease(midiNote);
                }
            }
        };

        window.addEventListener('keydown', handleKeyDown);
        window.addEventListener('keyup', handleKeyUp);

        return () => {
            window.removeEventListener('keydown', handleKeyDown);
            window.removeEventListener('keyup', handleKeyUp);
        };
    }, [wsClient, selectedCategory]);

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
        
        // Update visual feedback for pad clicks
        // Need to convert MIDI note back to padId (0-15)
        let visualPadId = padId;
        if (selectedCategory === 'drums') {
            visualPadId = padId - 36; // Drums: MIDI 36-51 → pad 0-15
        } else {
            // Notes/Bass: need to reverse the scale mapping (approximate)
            visualPadId = padId - 48; // Simplified - assumes linear for now
        }
        
        if (visualPadId >= 0 && visualPadId < 16) {
            setActivePads(prev => new Set(prev).add(visualPadId));
        }
        
        wsClient.send({ cmd: "NOTE_ON", pad: padId, val });
    };

    const handlePadRelease = (padId: number) => {
        console.log('[App] Pad release:', { padId });
        
        // Update visual feedback for pad releases
        let visualPadId = padId;
        if (selectedCategory === 'drums') {
            visualPadId = padId - 36;
        } else {
            visualPadId = padId - 48;
        }
        
        if (visualPadId >= 0 && visualPadId < 16) {
            setActivePads(prev => {
                const next = new Set(prev);
                next.delete(visualPadId);
                return next;
            });
        }
        
        wsClient.send({ cmd: "NOTE_OFF", pad: padId });
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

    const handlePanic = () => {
        console.log('[App] PANIC - Stopping all notes');
        wsClient.send({ cmd: "PANIC" });
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
        console.log('[App] Home / Jam Manager clicked - stopping session');
        // Stop playback when going home
        if (isPlaying) {
            wsClient.send({ cmd: "PAUSE" });
            setIsPlaying(false);
        }
        setShowJamManager(true);
    };

    const handleCreateJam = () => {
        console.log('[App] Create new jam');
        wsClient.send({ cmd: "NEW_JAM" });
        setShowJamManager(false);
        setActiveTab('mode');
    };

    const handleOpenJam = (jamId: string) => {
        console.log('[App] Opening jam:', jamId);
        wsClient.send({ cmd: "LOAD_JAM", sessionId: jamId });
        setShowJamManager(false);
        setActiveTab('play');
    };

    const handleRenameJam = (jamId: string, name: string, emoji?: string) => {
        console.log('[App] Renaming jam:', jamId, name, emoji);
        wsClient.send({ cmd: "RENAME_JAM", sessionId: jamId, name, emoji });
    };

    const handleDeleteJam = (jamId: string) => {
        console.log('[App] Deleting jam:', jamId);
        if (confirm('Are you sure you want to delete this jam? This action cannot be undone.')) {
            wsClient.send({ cmd: "DELETE_JAM", sessionId: jamId });
        }
    };

    const handleLoadRiff = (riffId: string) => {
        console.log('[App] Loading riff:', riffId);
        wsClient.send({ cmd: "LOAD_RIFF", riffId });
    };

    // Determine bottom content (Performance Surface, Mixer Controls, or Mic Controls)
    let bottomContent: React.ReactNode | undefined = undefined;
    
    if (activeTab === 'mixer') {
        bottomContent = <MixerControls
            state={state}
            onSlotVolumeChange={handleSlotVolumeChange}
        />;
    } else if (selectedCategory === 'mic') {
        // Mic mode always shows gain control in bottom section (even in Adjust tab)
        bottomContent = (
            <div style={{
                height: '100%',
                display: 'flex',
                flexDirection: 'row',
                alignItems: 'center',
                justifyContent: 'center',
                gap: 60,
                padding: 40
            }}>
                {/* Input Level Monitor (Read-only meter) */}
                <div style={{
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    gap: 12,
                    height: '100%'
                }}>
                    <div style={{
                        fontSize: 10,
                        fontWeight: 900,
                        color: 'var(--text-secondary)',
                        letterSpacing: '0.15em',
                        textAlign: 'center'
                    }}>
                        INPUT LEVEL
                    </div>
                    <div className="glass-panel" style={{
                        flex: 1,
                        width: 40,
                        position: 'relative',
                        background: 'rgba(0,0,0,0.3)',
                        border: '1px solid var(--glass-border)',
                        display: 'flex',
                        flexDirection: 'column',
                        justifyContent: 'flex-end'
                    }}>
                        {/* VU Meter */}
                        <div style={{
                            width: '100%',
                            height: `${(state?.mic?.inputLevel ?? 0) * 100}%`,
                            background: `linear-gradient(to top, var(--neon-green) 0%, #fff 100%)`,
                            opacity: 0.8,
                            transition: 'height 0.05s linear',
                            boxShadow: `0 0 15px var(--neon-green)`
                        }} />
                    </div>
                    <span style={{
                        fontSize: '10px',
                        color: 'var(--text-secondary)',
                        textTransform: 'uppercase',
                        fontWeight: 'bold',
                        letterSpacing: '0.05em'
                    }}>
                        INPUT
                    </span>
                </div>
                
                {/* Center Controls */}
                <div style={{
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    justifyContent: 'center',
                    gap: 30
                }}>
                    <div style={{
                        fontSize: 10,
                        fontWeight: 900,
                        color: 'var(--text-secondary)',
                        letterSpacing: '0.15em',
                        textAlign: 'center'
                    }}>
                        MICROPHONE INPUT
                    </div>
                    
                    {/* Large Gain Knob */}
                    <div className="interactive-element">
                        <Knob
                            label="GAIN"
                            value={state?.mic?.inputGain ?? 0.7}
                            onChange={(val) => {
                                // Convert 0-1 range to -60dB to +40dB
                                const dbValue = (val * 100) - 60; // Maps 0→-60dB, 1→+40dB
                                wsClient.send({ cmd: "SET_INPUT_GAIN", val: dbValue });
                            }}
                            size={150}
                            color="var(--neon-cyan)"
                        />
                    </div>
                    <div style={{ fontSize: 11, color: 'var(--text-secondary)' }}>
                        -60dB to +40dB
                    </div>
                    
                    <div style={{
                        display: 'flex',
                        flexDirection: 'column',
                        gap: 12,
                        width: '100%',
                        maxWidth: 300
                    }}>
                        <button
                            className="glass-panel interactive-element"
                            style={{
                                background: state?.mic?.monitorInput ? 'rgba(0, 229, 255, 0.2)' : 'var(--glass-bg)',
                                border: state?.mic?.monitorInput ? '1px solid var(--neon-cyan)' : '1px solid var(--glass-border)',
                                borderRadius: 8,
                                padding: 12,
                                color: state?.mic?.monitorInput ? 'var(--neon-cyan)' : '#fff',
                                fontSize: 11,
                                fontWeight: 'bold',
                                cursor: 'pointer'
                            }}
                            onClick={() => wsClient.send({ cmd: "TOGGLE_MONITOR_INPUT" })}
                        >
                            MONITOR INPUT
                        </button>
                        <button
                            className="glass-panel interactive-element"
                            style={{
                                background: state?.mic?.monitorUntilLooped ? 'rgba(0, 229, 255, 0.2)' : 'var(--glass-bg)',
                                border: state?.mic?.monitorUntilLooped ? '1px solid var(--neon-cyan)' : '1px solid var(--glass-border)',
                                borderRadius: 8,
                                padding: 12,
                                color: state?.mic?.monitorUntilLooped ? 'var(--neon-cyan)' : '#fff',
                                fontSize: 11,
                                fontWeight: 'bold',
                                cursor: 'pointer'
                            }}
                            onClick={() => wsClient.send({ cmd: "TOGGLE_MONITOR_UNTIL_LOOPED" })}
                        >
                            MONITOR UNTIL LOOPED
                        </button>
                    </div>
                </div>
            </div>
        );
    }

    // If showing Jam Manager, render it as full-screen home
    if (showJamManager) {
        return <JamManagerView
            sessions={state?.sessions || []}
            onCreateJam={handleCreateJam}
            onOpenJam={handleOpenJam}
            onRenameJam={handleRenameJam}
            onDeleteJam={handleDeleteJam}
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
            onPanic={handlePanic}
            performanceMode={performanceMode}
            onPadTrigger={handlePadTrigger}
            onPadRelease={handlePadRelease}
            onXYChange={handleXYChange}
            activeCategory={selectedCategory}
            activePads={activePads}
            bottomContent={bottomContent}
            onHomeClick={handleHomeClick}
            riffHistory={state?.riffHistory || []}
            onLoadRiff={handleLoadRiff}
            looperInputLevel={state?.looper?.inputLevel ?? 0}
        >
            {activeTab === 'mode' && <ModeView onSelectMode={handleSelectMode} />}
            {activeTab === 'play' && <PlayView
                state={{...state!, activeMode: {...state?.activeMode, category: selectedCategory}}}
                onSelectPreset={handleSelectPreset}
                selectedPreset={selectedPreset}
                category={selectedCategory}
            />}
            {activeTab === 'adjust' && <AdjustView
                onAdjustParam={handleAdjustParam}
                activeMode={selectedCategory}
            />}
            {activeTab === 'mixer' && <MixerView
                state={state}
                onToggleMetronome={handleToggleMetronome}
                onSlotVolumeChange={handleSlotVolumeChange}
                onSelectSlot={handleSelectSlot}
                onMoreSettings={() => {
                    console.log('[App] More settings clicked');
                    setShowSettings(true);
                }}
            />}
        </MainLayout>
    )
}

export default App
