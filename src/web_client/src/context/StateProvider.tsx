// context/StateProvider.tsx
import React, { createContext, useContext, useEffect, useState } from 'react';
import { AppState } from '../../../shared/protocol/schema';
import { WebSocketClient } from '../api/WebSocketClient';

const defaultState: AppState = {
    bpm: 120,
    isPlaying: false,
    activeRiffs: []
};

const StateContext = createContext<AppState>(defaultState);

export const StateProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [state, setState] = useState<AppState>(defaultState);
    const client = new WebSocketClient();

    useEffect(() => {
        client.connect((newState: AppState) => {
            setState(newState);
        });
    }, []);

    return (
        <StateContext.Provider value={state}>
            {children}
        </StateContext.Provider>
    );
};

export const useAppState = () => useContext(StateContext);
