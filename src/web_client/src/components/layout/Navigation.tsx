import React from 'react';
import { Icon } from '../shared/Icon';

export type TabId = 'mode' | 'play' | 'adjust' | 'mixer';

interface NavigationProps {
    activeTab: TabId;
    onTabChange: (tab: TabId) => void;
}

export const Navigation: React.FC<NavigationProps> = ({ activeTab, onTabChange }) => {
    const tabs: { id: TabId; label: string; icon: string }[] = [
        { id: 'mode', label: 'Mode', icon: 'grid' },
        { id: 'play', label: 'Play', icon: 'wave' },
        { id: 'adjust', label: 'Adjust', icon: 'settings' }, // using settings icon for knob as placeholder
        { id: 'mixer', label: 'Mixer', icon: 'sliders' },
    ];

    return (
        <div style={{
            height: 60,
            background: '#1a1a1a',
            borderTop: '1px solid #333',
            display: 'flex',
            justifyContent: 'space-around',
            alignItems: 'center',
            paddingBottom: 'env(safe-area-inset-bottom)'
        }}>
            {tabs.map(tab => {
                const isActive = activeTab === tab.id;
                return (
                    <button
                        key={tab.id}
                        onClick={() => onTabChange(tab.id)}
                        style={{
                            background: 'none',
                            border: 'none',
                            color: isActive ? '#00E5FF' : '#888',
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            gap: 4,
                            cursor: 'pointer'
                        }}
                    >
                        <Icon name={tab.icon} size={20} color={isActive ? '#00E5FF' : '#888'} />
                        <span style={{ fontSize: 10 }}>{tab.label}</span>
                    </button>
                )
            })}
        </div>
    );
};
