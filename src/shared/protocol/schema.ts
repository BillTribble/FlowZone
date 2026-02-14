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

export interface Riff {
    id: string;
    name: string;
    lengthBeats: number;
}

export interface AppState {
    bpm: number;
    isPlaying: boolean;
    activeRiffs: Riff[];
}

export interface Parameters {
    bpm: number;
}

// Command Message Structure
export interface CommandMessage {
    type: CommandType;
    payload?: any;
}
