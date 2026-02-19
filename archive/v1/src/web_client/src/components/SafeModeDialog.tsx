import React from 'react';

interface SafeModeDialogProps {
  safeModeLevel: number;
  description: string;
  onClearHistory: () => void;
  onContinue: () => void;
}

export const SafeModeDialog: React.FC<SafeModeDialogProps> = ({
  safeModeLevel,
  description,
  onClearHistory,
  onContinue,
}) => {
  if (safeModeLevel === 0) {
    return null; // No safe mode active
  }

  return (
    <div className="safe-mode-dialog-overlay">
      <div className="safe-mode-dialog">
        <h1>⚠️ Safe Mode Active</h1>
        
        <div className="safe-mode-content">
          <div className={`safe-mode-level level-${safeModeLevel}`}>
            Level {safeModeLevel}
          </div>
          
          <p className="description">{description}</p>
          
          {safeModeLevel === 1 && (
            <div className="info">
              <p>VST plugins have been disabled due to repeated crashes.</p>
              <p>FlowZone will continue to work without plugins.</p>
            </div>
          )}
          
          {safeModeLevel === 2 && (
            <div className="info">
              <p>Audio device has been reset due to driver failure.</p>
              <p>Please check your audio device settings.</p>
            </div>
          )}
          
          {safeModeLevel === 3 && (
            <div className="info warning">
              <p><strong>Critical:</strong> Multiple crashes detected.</p>
              <p>We recommend resetting to factory defaults.</p>
            </div>
          )}
        </div>
        
        <div className="safe-mode-actions">
          {safeModeLevel === 3 && (
            <button 
              className="button-primary"
              onClick={onClearHistory}
            >
              Reset to Factory Defaults
            </button>
          )}
          
          <button 
            className="button-secondary"
            onClick={onClearHistory}
          >
            Clear Crash History
          </button>
          
          <button 
            className="button-tertiary"
            onClick={onContinue}
          >
            Continue Anyway
          </button>
        </div>
      </div>
    </div>
  );
};

export default SafeModeDialog;
