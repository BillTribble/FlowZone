// protocol/schema.ts

export enum ErrorCode {
    None = 0,
    InvalidCommand = 1001,
    InvalidPayload = 1002,
    EngineNotReady = 2001,
    AudioDeviceError = 3001
}

export enum CommandType {
    Play = "Play",
    Pause = "Pause",
    SetBpm = "SetBpm",
    LoadRiff = "LoadRiff"
}

export interface PluginInstance {
    id: string;
    pluginId: string;
    manufacturer: string;
    name: string;
    bypass: boolean;
    state: string; // Base64
}

export interface SlotState {
    id: string;
    state: "EMPTY" | "PLAYING" | "MUTED";
    volume: number;
    name: string;
    instrumentCategory: string;
    presetId: string;
    userId: string;
    pluginChain: PluginInstance[];
    loopLengthBars: number;
    originalBpm: number;
    lastError: number;
}

export interface RiffHistoryEntry {
    id: string;
    timestamp: number;
    name: string;
    layers: number;
    colors: string[];
    userId: string;
}

export interface AppState {
    sessions: Array<{
        id: string;
        name: string;
        emoji: string;
        createdAt: number;
    }>;
    session: {
        id: string;
        name: string;
        emoji: string;
        createdAt: number;
    };
    transport: {
        bpm: number;
        isPlaying: boolean;
        barPhase: number;
        loopLengthBars: number;
        metronomeEnabled: boolean;
        quantiseEnabled: boolean;
        rootNote: number;
        scale: string;
    };
    activeMode: {
        category: string;
        presetId: string;
        presetName: string;
        isFxMode: boolean;
        selectedSourceSlots: number[];
    };
    activeFX: {
        effectId: string;
        effectName: string;
        xyPosition: {
            x: number;
            y: number;
        };
        isActive: boolean;
    };
    mic: {
        inputGain: number;
        monitorInput: boolean;
        monitorUntilLooped: boolean;
    };
    slots: SlotState[];
    riffHistory: RiffHistoryEntry[];
    settings: {
        riffSwapMode: string;
        bufferSize: number;
        sampleRate: number;
        storageLocation: string;
    };
    system: {
        cpuLoad: number;
        diskBufferUsage: number;
        memoryUsageMB: number;
        activePluginHosts: number;
    };
    ui: any; // Empty object per spec
}
