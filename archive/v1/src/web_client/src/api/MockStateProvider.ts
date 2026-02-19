import { AppState } from '../../../shared/protocol/schema';

export const initialMockState: AppState = {
    sessions: [],
    session: {
        id: "mock-session-id",
        name: "Mock Session",
        emoji: "🎸",
        createdAt: Date.now()
    },
    looper: {
        inputLevel: 0,
        waveformData: new Array(256).fill(0)
    },
    transport: {
        bpm: 120.0,
        isPlaying: false,
        barPhase: 0.0,
        loopLengthBars: 4,
        metronomeEnabled: false,
        quantiseEnabled: true,
        rootNote: 0,
        scale: "chromatic"
    },
    activeMode: {
        category: "drums",
        presetId: "standard-kit",
        presetName: "Standard Kit",
        isFxMode: false,
        selectedSourceSlots: []
    },
    activeFX: {
        effectId: "none",
        effectName: "None",
        xyPosition: { x: 0.5, y: 0.5 },
        isActive: false
    },
    mic: {
        inputGain: 0.5,
        inputLevel: 0.0,
        monitorInput: true,
        monitorUntilLooped: true
    },
    slots: [
        {
            id: "slot-1",
            state: "EMPTY",
            volume: 1.0,
            name: "Vocals",
            instrumentCategory: "voice",
            presetId: "default",
            userId: "user-1",
            pluginChain: [],
            loopLengthBars: 4,
            originalBpm: 120.0,
            lastError: 0
        },
        {
            id: "slot-2",
            state: "PLAYING",
            volume: 0.8,
            name: "Guitar",
            instrumentCategory: "guitar",
            presetId: "clean",
            userId: "user-1",
            pluginChain: [],
            loopLengthBars: 4,
            originalBpm: 120.0,
            lastError: 0
        }
    ],
    riffHistory: [],
    settings: {
        riffSwapMode: "instant",
        bufferSize: 512,
        sampleRate: 44100,
        storageLocation: "/tmp/flowzone"
    },
    system: {
        cpuLoad: 0.1,
        diskBufferUsage: 0.05,
        memoryUsageMB: 150,
        activePluginHosts: 0
    },
    ui: {}
};

export class MockStateProvider {
    private state: AppState;
    private listeners: ((state: AppState) => void)[] = [];

    constructor() {
        this.state = JSON.parse(JSON.stringify(initialMockState));
        // Simulate transport update
        setInterval(() => {
            if (this.state.transport.isPlaying) {
                this.state.transport.barPhase = (this.state.transport.barPhase + 0.01) % 1.0;
                this.notify();
            }
        }, 50);
    }

    getState(): AppState {
        return this.state;
    }

    subscribe(callback: (state: AppState) => void): () => void {
        this.listeners.push(callback);
        return () => {
            this.listeners = this.listeners.filter(cb => cb !== callback);
        };
    }

    private notify() {
        this.listeners.forEach(cb => cb(this.state));
    }

    // Mock commands
    togglePlay() {
        this.state.transport.isPlaying = !this.state.transport.isPlaying;
        this.notify();
    }
}

export const mockStateProvider = new MockStateProvider();
