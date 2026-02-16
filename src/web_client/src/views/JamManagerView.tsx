import React, { useState } from 'react';
import { Icon } from '../components/shared/Icon';

interface Session {
    id: string;
    name: string;
    emoji: string;
    createdAt: number;
}

interface JamManagerViewProps {
    sessions: Session[];
    onCreateJam: () => void;
    onOpenJam: (jamId: string) => void;
    onRenameJam: (jamId: string, name: string, emoji?: string) => void;
    onDeleteJam: (jamId: string) => void;
}

export const JamManagerView: React.FC<JamManagerViewProps> = ({
    sessions,
    onCreateJam,
    onOpenJam,
    onRenameJam,
    onDeleteJam
}) => {
    const [editingId, setEditingId] = useState<string | null>(null);
    const [editName, setEditName] = useState('');
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
                {sessions.map(jam => {
                    const isEditing = editingId === jam.id;
                    const jamDate = new Date(jam.createdAt).toLocaleDateString('en-GB', {
                        day: '2-digit',
                        month: 'short',
                        year: 'numeric'
                    });
                    
                    return (
                        <div
                            key={jam.id}
                            className="glass-panel"
                            style={{
                                background: 'var(--glass-bg)',
                                border: '1px solid var(--glass-border)',
                                borderRadius: 12,
                                padding: 20,
                                textAlign: 'left',
                                display: 'flex',
                                flexDirection: 'column',
                                gap: 12,
                                transition: 'all 0.2s ease',
                                minHeight: 140,
                                position: 'relative'
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
                                    {isEditing ? (
                                        <input
                                            type="text"
                                            value={editName}
                                            onChange={(e) => setEditName(e.target.value)}
                                            onKeyDown={(e) => {
                                                if (e.key === 'Enter') {
                                                    onRenameJam(jam.id, editName);
                                                    setEditingId(null);
                                                } else if (e.key === 'Escape') {
                                                    setEditingId(null);
                                                }
                                            }}
                                            onBlur={() => {
                                                if (editName.trim()) {
                                                    onRenameJam(jam.id, editName);
                                                }
                                                setEditingId(null);
                                            }}
                                            autoFocus
                                            style={{
                                                fontSize: 16,
                                                fontWeight: 'bold',
                                                color: '#fff',
                                                background: 'rgba(0,0,0,0.3)',
                                                border: '1px solid var(--neon-cyan)',
                                                borderRadius: 4,
                                                padding: '4px 8px',
                                                width: '100%'
                                            }}
                                        />
                                    ) : (
                                        <div
                                            onClick={() => {
                                                setEditingId(jam.id);
                                                setEditName(jam.name);
                                            }}
                                            style={{
                                                fontSize: 16,
                                                fontWeight: 'bold',
                                                color: '#fff',
                                                marginBottom: 4,
                                                cursor: 'text'
                                            }}
                                        >
                                            {jam.name}
                                        </div>
                                    )}
                                    <div style={{
                                        fontSize: 11,
                                        color: 'var(--text-secondary)',
                                        fontFamily: 'monospace'
                                    }}>
                                        {jamDate}
                                    </div>
                                </div>
                            </div>

                            {/* Action Buttons */}
                            <div style={{
                                display: 'flex',
                                gap: 8,
                                marginTop: 'auto'
                            }}>
                                <button
                                    onClick={() => onOpenJam(jam.id)}
                                    className="interactive-element"
                                    style={{
                                        flex: 1,
                                        background: 'rgba(0, 229, 255, 0.15)',
                                        border: '1px solid var(--neon-cyan)',
                                        borderRadius: 6,
                                        padding: '8px 12px',
                                        color: 'var(--neon-cyan)',
                                        fontWeight: 'bold',
                                        fontSize: 11,
                                        cursor: 'pointer'
                                    }}
                                >
                                    OPEN
                                </button>
                                <button
                                    onClick={() => onDeleteJam(jam.id)}
                                    className="interactive-element"
                                    style={{
                                        background: 'rgba(255, 0, 0, 0.1)',
                                        border: '1px solid rgba(255, 0, 0, 0.3)',
                                        borderRadius: 6,
                                        padding: '8px 12px',
                                        color: '#ff6666',
                                        fontWeight: 'bold',
                                        fontSize: 11,
                                        cursor: 'pointer'
                                    }}
                                >
                                    <Icon name="trash" size={14} color="#ff6666" />
                                </button>
                            </div>
                        </div>
                    );
                })}
            </div>

            {/* Empty State */}
            {sessions.length === 0 && (
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
