import React from 'react';
import { Icon } from '../components/shared/Icon';

interface JamManagerViewProps {
    onCreateJam: () => void;
    onOpenJam: (jamId: string) => void;
}

// Mock jam data - will be replaced with real data from state
const MOCK_JAMS = [
    { id: 'jam-1', name: 'Morning Session', emoji: '🌅', date: '2026-02-15', layers: 4 },
    { id: 'jam-2', name: 'Groove Experiment', emoji: '🎵', date: '2026-02-14', layers: 6 },
    { id: 'jam-3', name: 'Bass Test', emoji: '🎸', date: '2026-02-13', layers: 2 },
];

export const JamManagerView: React.FC<JamManagerViewProps> = ({ onCreateJam, onOpenJam }) => {
    return (
        <div style={{
            height: '100%',
            display: 'flex',
            flexDirection: 'column',
            padding: 24,
            gap: 24,
            overflow: 'auto'
        }}>
            {/* Header */}
            <div style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center'
            }}>
                <div>
                    <h1 style={{ 
                        fontSize: 28, 
                        fontWeight: 900, 
                        margin: 0,
                        background: 'linear-gradient(135deg, var(--neon-cyan), var(--neon-pink))',
                        WebkitBackgroundClip: 'text',
                        WebkitTextFillColor: 'transparent',
                        letterSpacing: '-0.02em'
                    }}>
                        Jam Manager
                    </h1>
                    <p style={{ 
                        fontSize: 12, 
                        color: 'var(--text-secondary)', 
                        margin: '4px 0 0 0' 
                    }}>
                        Select a jam to continue or create a new one
                    </p>
                </div>
                
                <button
                    onClick={onCreateJam}
                    className="glass-panel interactive-element neon-glow"
                    style={{
                        background: 'rgba(0, 229, 255, 0.15)',
                        border: '2px solid var(--neon-cyan)',
                        borderRadius: 12,
                        padding: '12px 24px',
                        color: 'var(--neon-cyan)',
                        fontWeight: 900,
                        fontSize: 14,
                        cursor: 'pointer',
                        display: 'flex',
                        alignItems: 'center',
                        gap: 8,
                        letterSpacing: '0.05em'
                    }}
                >
                    <Icon name="plus" size={18} color="var(--neon-cyan)" />
                    NEW JAM
                </button>
            </div>

            {/* Jam Grid */}
            <div style={{
                flex: 1,
                display: 'grid',
                gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))',
                gap: 16,
                alignContent: 'start'
            }}>
                {MOCK_JAMS.map(jam => (
                    <button
                        key={jam.id}
                        onClick={() => onOpenJam(jam.id)}
                        className="glass-panel interactive-element"
                        style={{
                            background: 'var(--glass-bg)',
                            border: '1px solid var(--glass-border)',
                            borderRadius: 12,
                            padding: 20,
                            cursor: 'pointer',
                            textAlign: 'left',
                            display: 'flex',
                            flexDirection: 'column',
                            gap: 12,
                            transition: 'all 0.2s ease',
                            minHeight: 140
                        }}
                    >
                        {/* Emoji + Name */}
                        <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                            <div style={{ 
                                fontSize: 32,
                                width: 48,
                                height: 48,
                                display: 'flex',
                                alignItems: 'center',
                                justifyContent: 'center',
                                background: 'rgba(255,255,255,0.05)',
                                borderRadius: 8
                            }}>
                                {jam.emoji}
                            </div>
                            <div style={{ flex: 1 }}>
                                <div style={{ 
                                    fontSize: 16, 
                                    fontWeight: 'bold',
                                    color: '#fff',
                                    marginBottom: 4
                                }}>
                                    {jam.name}
                                </div>
                                <div style={{ 
                                    fontSize: 11, 
                                    color: 'var(--text-secondary)',
                                    fontFamily: 'monospace'
                                }}>
                                    {jam.date}
                                </div>
                            </div>
                        </div>

                        {/* Stats */}
                        <div style={{
                            display: 'flex',
                            gap: 16,
                            fontSize: 11,
                            color: 'var(--text-secondary)'
                        }}>
                            <div style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
                                <Icon name="layers" size={14} color="var(--text-secondary)" />
                                <span>{jam.layers} layers</span>
                            </div>
                        </div>

                        {/* Visual separator */}
                        <div style={{
                            height: 2,
                            background: 'linear-gradient(90deg, var(--neon-cyan), transparent)',
                            opacity: 0.3,
                            borderRadius: 1
                        }} />
                    </button>
                ))}
            </div>

            {/* Empty State */}
            {MOCK_JAMS.length === 0 && (
                <div style={{
                    flex: 1,
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    justifyContent: 'center',
                    gap: 16,
                    color: 'var(--text-secondary)',
                    padding: 40
                }}>
                    <Icon name="grid" size={48} color="var(--text-secondary)" />
                    <p style={{ fontSize: 14, margin: 0 }}>No jams yet</p>
                    <p style={{ fontSize: 12, margin: 0, opacity: 0.7 }}>
                        Create your first jam to get started
                    </p>
                </div>
            )}
        </div>
    );
};
