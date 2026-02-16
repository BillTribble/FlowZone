import React from 'react';
import { Icon } from '../shared/Icon';

export type TabId = 'jam-manager' | 'mode' | 'play' | 'adjust' | 'mixer';

interface NavigationProps {
    activeTab: TabId;
    onTabChange: (tab: TabId) => void;
}

export const Navigation: React.FC<NavigationProps> = ({ activeTab, onTabChange }) => {
    const tabs: { id: TabId; label: string; icon: string }[] = [
        { id: 'jam-manager', label: 'JAMS', icon: 'home' },
        { id: 'mode', label: 'MODE', icon: 'grid' },
        { id: 'play', label: 'PLAY', icon: 'wave' },
        { id: 'adjust', label: 'ADJUST', icon: 'settings' },
        { id: 'mixer', label: 'MIXER', icon: 'sliders' },
    ];

    return (
        <div style={{
            height: 64,
            background: 'rgba(10, 10, 11, 0.95)',
            borderTop: '1px solid var(--glass-border)',
            display: 'flex',
            justifyContent: 'space-around',
            alignItems: 'center',
            paddingBottom: 'env(safe-area-inset-bottom)',
            backdropFilter: 'blur(20px)',
            boxShadow: '0 -4px 20px rgba(0,0,0,0.5)'
        }}>
            {tabs.map(tab => {
                const isActive = activeTab === tab.id;
                return (
                    <button
                        key={tab.id}
                        onClick={() => onTabChange(tab.id)}
                        className="interactive-element"
                        style={{
                            background: 'none',
                            border: 'none',
                            color: isActive ? 'var(--neon-cyan)' : 'var(--text-secondary)',
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            gap: 4,
                            cursor: 'pointer',
                            width: 80,
                            padding: '8px 0',
                            transition: 'color 0.2s ease'
                        }}
                    >
                        <div style={{
                            transform: isActive ? 'scale(1.1)' : 'scale(1)',
                            filter: isActive ? 'drop-shadow(0 0 8px var(--neon-cyan))' : 'none',
                            transition: 'transform 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275)'
                        }}>
                            <Icon name={tab.icon} size={18} color={isActive ? 'var(--neon-cyan)' : 'var(--text-secondary)'} />
                        </div>
                        <span style={{
                            fontSize: '9px',
                            fontWeight: 900,
                            letterSpacing: '0.1em',
                            opacity: isActive ? 1 : 0.6
                        }}>
                            {tab.label}
                        </span>
                    </button>
                )
            })}
        </div>
    );
};
