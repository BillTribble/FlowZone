import React, { useState, useEffect, useCallback } from 'react'
import { WebSocketClient } from './api/WebSocketClient'
import { flowLogger } from './api/FlowLogger'
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

function App() {
    const [state, setState] = useState<AppState | null>(null)
    const [connected, setConnected] = useState(false)
    const [showJamManager, setShowJamManager] = useState(true) // Start at home screen
    const [showSettings, setShowSettings] = useState(false);
    const [activeTab, setActiveTab] = useState<TabId>('mode')
    const [selectedPreset, setSelectedPreset] = useState<string | null>(null)
    const [selectedCategory, setSelectedCategory] = useState<string>('drums')
    const [isMixDirty, setIsMixDirty] = useState(false)

    // Initialize client once
    const [wsClient] = useState(() => new WebSocketClient('ws://localhost:50001'))

    // Helper to apply JSON Patch (RFC 6902)
    const applyPatch = (state: AppState, ops: any[]): AppState => {
        const newState = JSON.parse(JSON.stringify(state)); // Deep clone
        for (const op of ops) {
            const pathParts = op.path.split('/').filter((p: string) => p !== '');
            let current = newState;
            for (let i = 0; i < pathParts.length - 1; i++) {
                const part = pathParts[i];
                if (current[part] === undefined) {
                    if (op.op === 'add') {
                        current[part] = isNaN(Number(pathParts[i + 1])) ? {} : [];
                    } else break;
                }
                current = current[part];
            }

            const lastPart = pathParts[pathParts.length - 1];
            if (op.op === 'add' || op.op === 'replace') {
                if (Array.isArray(current) && lastPart === '-') {
                    current.push(op.value);
                } else {
                    current[lastPart] = op.value;
                }
            } else if (op.op === 'remove') {
                if (Array.isArray(current)) {
                    current.splice(parseInt(lastPart), 1);
                } else {
                    delete current[lastPart];
                }
            }
        }
        return newState;
    };

    useEffect(() => {
        console.log('[App] Initializing WebSocket connection...');
        wsClient.connect((message) => {
            console.log('[App] Message received:', message.type);

            if (message.type === 'STATE_FULL') {
                console.log('[App] Full state update received:', message.data);
                flowLogger.log('STATE', `STATE_FULL received, keys=${Object.keys(message.data || {}).join(',')}`);
                if (message.data) {
                    flowLogger.log('AUDIO', `INITIAL mic.inputLevel=${message.data.mic?.inputLevel} looper.inputLevel=${message.data.looper?.inputLevel}`);
                }
                setState(message.data);
                setConnected(true);
            } else if (message.type === 'STATE_PATCH') {
                setState((prevState) => {
                    if (!prevState) return null;
                    const nextState = applyPatch(prevState, message.ops);
                    // Sampled logging for state patches
                    flowLogger.logSampled('AUDIO', 'state_patch',
                        `mic.inputLevel=${nextState.mic?.inputLevel?.toFixed(4)} looper.inputLevel=${nextState.looper?.inputLevel?.toFixed(4)} waveformNonZero=${nextState.looper?.waveformData?.some((v: number) => v > 0.001) ? 'YES' : 'NO'}`,
                        60);
                    return nextState;
                });
            } else {
                // Legacy / Direct handle
                console.log('[App] Direct state update received:', message);
                flowLogger.log('STATE', `DIRECT state update`);
                setState(message);
                setConnected(true);
            }
        })
        return () => {
            // wsClient.disconnect()
        }
    }, [wsClient])

    // Track active pads for visual feedback
    const [activePads, setActivePads] = useState<Set<number>>(new Set());
    const [performanceMode, setPerformanceMode] = useState<'PADS' | 'XY'>('PADS');
    const [isPlaying, setIsPlaying] = useState(false);

    // WebSocket Handlers - use useCallback to ensure stable references for keyboard handler
    const handlePadTrigger = useCallback((padId: number, val: number) => {
        console.log('[App] Pad trigger:', { padId, val });

        // Update visual feedback
        let visualPadId = -1;
        if (selectedCategory === 'drums') {
            visualPadId = padId - 36;
        } else {
            const baseNote = 48;
            const intervals = [0, 2, 4, 5, 7, 9, 11]; // major scale
            const numSteps = intervals.length;

            for (let pad = 0; pad < 16; pad++) {
                const octave = Math.floor(pad / numSteps);
                const degree = pad % numSteps;
                const expectedMidi = baseNote + (octave * 12) + intervals[degree];
                if (expectedMidi === padId) {
                    visualPadId = pad;
                    break;
                }
            }
        }

        if (visualPadId >= 0 && visualPadId < 16) {
            setActivePads(prev => new Set(prev).add(visualPadId));
        }

        wsClient.send({ cmd: "NOTE_ON", pad: padId, val });
    }, [wsClient, selectedCategory]);

    const handlePadRelease = useCallback((padId: number) => {
        // Update visual feedback
        let visualPadId = -1;
        if (selectedCategory === 'drums') {
            visualPadId = padId - 36;
        } else {
            const baseNote = 48;
            const intervals = [0, 2, 4, 5, 7, 9, 11];
            const numSteps = intervals.length;

            for (let pad = 0; pad < 16; pad++) {
                const octave = Math.floor(pad / numSteps);
                const degree = pad % numSteps;
                const expectedMidi = baseNote + (octave * 12) + intervals[degree];
                if (expectedMidi === padId) {
                    visualPadId = pad;
                    break;
                }
            }
        }

        if (visualPadId >= 0 && visualPadId < 16) {
            setActivePads(prev => {
                const next = new Set(prev);
                next.delete(visualPadId);
                return next;
            });
        }

        wsClient.send({ cmd: "NOTE_OFF", pad: padId });
    }, [wsClient, selectedCategory]);

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
                return baseNote + padId;
            } else {
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
            if (activeKeys.has(key)) return;
            activeKeys.add(key);

            if (key in keyToPadIndex) {
                e.preventDefault();
                const padIndex = keyToPadIndex[key];
                const baseNote = selectedCategory === 'drums' ? 36 : 48;
                const midiNote = getMidiNote(padIndex, baseNote);
                setActivePads(prev => new Set(prev).add(padIndex));
                handlePadTrigger(midiNote, 1.0);
                return;
            }

            const loopLengths: { [key: string]: number } = { '1': 1, '2': 2, '3': 4, '4': 8 };
            if (key in loopLengths) {
                e.preventDefault();
                wsClient.send({ cmd: "SET_LOOP_LENGTH", bars: loopLengths[key] });
            }
        };

        const handleKeyUp = (e: KeyboardEvent) => {
            const key = e.key.toLowerCase();
            activeKeys.delete(key);

            if (key in keyToPadIndex) {
                const padIndex = keyToPadIndex[key];
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
    }, [handlePadTrigger, handlePadRelease, wsClient, selectedCategory]);

    if (!state) {
        return (
            <div style={{
                height: '100vh',
                background: '#121212',
                color: '#fff',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontFamily: 'sans-serif',
                textAlign: 'center',
                padding: '20px'
            }}>
                <div>
                    <h1 style={{ marginBottom: '20px', color: 'var(--neon-cyan, #00e5ff)' }}>FlowZone</h1>
                    <div className="glass-panel" style={{ padding: '40px', borderRadius: '16px', border: '1px solid rgba(255,255,255,0.1)', background: 'rgba(255,255,255,0.05)' }}>
                        <p style={{ fontSize: '18px', marginBottom: '10px' }}>Connecting to Engine...</p>
                        <p style={{ color: '#888', fontSize: '14px', maxWidth: '400px' }}>
                            Ensuring connection to the FlowZone audio engine on port 50001.
                        </p>
                        {!connected && (
                            <p style={{ marginTop: '20px', color: '#666', fontSize: '12px' }}>
                                If this persists, ensure the FlowZone application is running.
                            </p>
                        )}
                    </div>
                </div>
            </div>
        )
    }

    const handleXYChange = (x: number, y: number) => {
        wsClient.send({ cmd: "XY_CHANGE", x, y });
    };

    const handleSelectPreset = (category: string, preset: string) => {
        setSelectedPreset(preset);
        wsClient.send({ cmd: "SET_PRESET", category, preset });
        if (!isPlaying) {
            setIsPlaying(true);
            wsClient.send({ cmd: "TOGGLE_PLAY" });
        }
    };

    const handleToggleMetronome = () => {
        wsClient.send({ cmd: "TOGGLE_METRONOME" });
    };

    const handleSlotVolumeChange = (slotId: number, volume: number) => {
        setIsMixDirty(true);
        wsClient.send({ cmd: "SET_SLOT_VOLUME", slot: slotId, volume });
    };

    const handleSelectSlot = (slotId: number) => {
        wsClient.send({ cmd: "SELECT_SLOT", slot: slotId });
    };

    const handlePanic = () => {
        wsClient.send({ cmd: "PANIC" });
    };

    const handleSelectMode = (mode: string) => {
        setSelectedCategory(mode);
        setSelectedPreset(null); // Reset selected preset on mode change to fix highlighting
        wsClient.send({ cmd: "SET_MODE", category: mode });
        if (mode === 'fx' || mode === 'ext_fx') {
            setPerformanceMode('XY');
        } else {
            setPerformanceMode('PADS');
        }
        setActiveTab('play');
    };

    const handleAdjustParam = (id: string, value: number) => {
        const paramId = parseInt(id) || 0;
        wsClient.send({ cmd: "ADJUST_PARAM", param: paramId, value });
    };

    const handleHomeClick = () => {
        wsClient.send({ cmd: "PAUSE" });
        setIsPlaying(false);
        setShowJamManager(true);
    };

    const handleCreateJam = () => {
        wsClient.send({ cmd: "NEW_JAM" });
        setShowJamManager(false);
        setActiveTab('mode');
    };

    const handleOpenJam = (jamId: string) => {
        wsClient.send({ cmd: "LOAD_JAM", sessionId: jamId });
        setShowJamManager(false);
        setActiveTab('play');
    };

    const handleRenameJam = (jamId: string, name: string, emoji?: string) => {
        wsClient.send({ cmd: "RENAME_JAM", sessionId: jamId, name, emoji });
    };

    const handleDeleteJam = (jamId: string) => {
        if (confirm('Are you sure you want to delete this jam? This action cannot be undone.')) {
            wsClient.send({ cmd: "DELETE_JAM", sessionId: jamId });
        }
    };

    const handleLoadRiff = (riffId: string) => {
        wsClient.send({ cmd: "LOAD_RIFF", riffId });
    };

    // Determine bottom content
    let bottomContent: React.ReactNode | undefined = undefined;

    if (activeTab === 'mixer') {
        bottomContent = <MixerControls
            state={state}
            onSlotVolumeChange={handleSlotVolumeChange}
            isMixDirty={isMixDirty}
        />;
    } else if (selectedCategory === 'mic') {
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
                        <div style={{
                            width: '100%',
                            height: `${(state?.mic?.inputLevel ?? 0) * 100}%`,
                            background: `linear-gradient(to top, var(--neon-green) 0%, #fff 100%)`,
                            opacity: 0.8,
                            boxShadow: `0 0 15px var(--neon-green)`
                        }} />
                    </div>
                    <span style={{ fontSize: '10px', color: 'var(--text-secondary)', textTransform: 'uppercase', fontWeight: 'bold' }}>
                        INPUT
                    </span>
                </div>

                <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 30 }}>
                    <div style={{ fontSize: 10, fontWeight: 900, color: 'var(--text-secondary)', letterSpacing: '0.15em' }}>
                        MICROPHONE INPUT
                    </div>
                    <Knob
                        label="GAIN"
                        value={state?.mic?.inputGain ?? 0.7}
                        onChange={(val) => {
                            const dbValue = (val * 100) - 60;
                            wsClient.send({ cmd: "SET_INPUT_GAIN", val: dbValue });
                        }}
                        size={150}
                        color="var(--neon-cyan)"
                    />
                    <div style={{ display: 'flex', flexDirection: 'column', gap: 12, width: '100%', maxWidth: 300 }}>
                        <button
                            className="glass-panel interactive-element"
                            style={{
                                background: state?.mic?.monitorInput ? 'rgba(0, 229, 255, 0.2)' : 'var(--glass-bg)',
                                border: state?.mic?.monitorInput ? '1px solid var(--neon-cyan)' : '1px solid var(--glass-border)',
                                borderRadius: 8, padding: 12, color: state?.mic?.monitorInput ? 'var(--neon-cyan)' : '#fff',
                                fontSize: 11, fontWeight: 'bold'
                            }}
                            onClick={() => wsClient.send({ cmd: "TOGGLE_MONITOR_INPUT" })}
                        >
                            MONITOR INPUT
                        </button>
                    </div>
                </div>
            </div>
        );
    }

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
            waveformData={state?.looper?.waveformData ? new Float32Array(state.looper.waveformData) : undefined}
        >
            {activeTab === 'mode' && <ModeView onSelectMode={handleSelectMode} />}
            {activeTab === 'play' && <PlayView
                state={{ ...state!, activeMode: { ...state?.activeMode, category: selectedCategory } }}
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
                onSelectSlot={handleSelectSlot}
                onMoreSettings={() => setShowSettings(true)}
            />}

            {showSettings && (
                <div className="settings-takeover">
                    <SettingsPanel onClose={() => setShowSettings(false)} />
                </div>
            )}
        </MainLayout>
    );
}

export default App
