import React, { createContext, useContext, useEffect, useState } from 'react';
import { AppState } from '../../../shared/protocol/schema';
import { initialMockState } from '../api/MockStateProvider';
import { WebSocketClient } from '../api/WebSocketClient';

const StateContext = createContext<AppState>(initialMockState);

export const StateProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [state, setState] = useState<AppState>(initialMockState);
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
