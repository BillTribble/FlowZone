import React, { useState } from 'react';

interface Tab {
  id: string;
  label: string;
}

const tabs: Tab[] = [
  { id: 'interface', label: 'Interface' },
  { id: 'audio', label: 'Audio' },
  { id: 'midi', label: 'MIDI & Sync' },
  { id: 'library', label: 'Library & VST' },
];

interface SettingsPanelProps {
  onClose: () => void;
}

export const SettingsPanel: React.FC<SettingsPanelProps> = ({ onClose }) => {
  const [activeTab, setActiveTab] = useState('interface');
  const [theme, setTheme] = useState('dark');
  const [audioDevice, setAudioDevice] = useState('');
  const [sampleRate, setSampleRate] = useState(44100);
  const [bufferSize, setBufferSize] = useState(512);
  const [vst3Paths, setVst3Paths] = useState<string[]>([]);
  const [riffSwapMode, setRiffSwapMode] = useState('quantized');

  const handleThemeToggle = () => {
    const newTheme = theme === 'dark' ? 'light' : 'dark';
    setTheme(newTheme);
    // Update CSS variables
    document.documentElement.setAttribute('data-theme', newTheme);
    console.log('Theme changed to:', newTheme);
  };

  const handleAudioDeviceChange = (device: string) => {
    setAudioDevice(device);
    // Fire SET_AUDIO_DEVICE command
    console.log('SET_AUDIO_DEVICE:', device);
  };

  const handleSampleRateChange = (rate: number) => {
    setSampleRate(rate);
    // Fire SET_SAMPLE_RATE command
    console.log('SET_SAMPLE_RATE:', rate);
  };

  const handleBufferSizeChange = (size: number) => {
    setBufferSize(size);
    // Fire SET_BUFFER_SIZE command
    console.log('SET_BUFFER_SIZE:', size);
  };

  const handleAddVST3Path = (path: string) => {
    setVst3Paths([...vst3Paths, path]);
    console.log('Added VST3 path:', path);
  };

  const handleRemoveVST3Path = (index: number) => {
    const newPaths = vst3Paths.filter((_, i) => i !== index);
    setVst3Paths(newPaths);
    console.log('Removed VST3 path at index:', index);
  };

  const handleRiffSwapModeChange = (mode: string) => {
    setRiffSwapMode(mode);
    console.log('Riff Swap Mode:', mode);
  };

  return (
    <div className="settings-panel">
      <div className="settings-header">
        <button className="back-button" onClick={onClose}>
          ← Back
        </button>
        <div className="settings-tabs">
          {tabs.map((tab) => (
            <button
              key={tab.id}
              className={`tab ${activeTab === tab.id ? 'active' : ''}`}
              onClick={() => setActiveTab(tab.id)}
            >
              {tab.label}
            </button>
          ))}
        </div>
      </div>

      <div className="settings-content">
        {activeTab === 'interface' && (
          <div className="tab-content">
            <h2>Interface Settings</h2>

            <div className="setting-row">
              <label>Theme</label>
              <button onClick={handleThemeToggle}>
                Toggle Theme (Current: {theme})
              </button>
            </div>

            <div className="setting-row">
              <label>Riff Swap Mode</label>
              <div className="radio-group">
                <label>
                  <input
                    type="radio"
                    name="riffSwapMode"
                    value="immediate"
                    checked={riffSwapMode === 'immediate'}
                    onChange={(e) => handleRiffSwapModeChange(e.target.value)}
                  />
                  Immediate
                </label>
                <label>
                  <input
                    type="radio"
                    name="riffSwapMode"
                    value="quantized"
                    checked={riffSwapMode === 'quantized'}
                    onChange={(e) => handleRiffSwapModeChange(e.target.value)}
                  />
                  Quantized (bar boundary)
                </label>
              </div>
            </div>
          </div>
        )}

        {activeTab === 'audio' && (
          <div className="tab-content">
            <h2>Audio Settings</h2>

            <div className="setting-row">
              <label>Audio Device</label>
              <select
                value={audioDevice}
                onChange={(e) => handleAudioDeviceChange(e.target.value)}
              >
                <option value="">Select Device...</option>
                <option value="CoreAudio">CoreAudio</option>
                <option value="Built-in">Built-in Output</option>
              </select>
            </div>

            <div className="setting-row">
              <label>Sample Rate</label>
              <select
                value={sampleRate}
                onChange={(e) => handleSampleRateChange(Number(e.target.value))}
              >
                <option value={44100}>44.1 kHz</option>
                <option value={48000}>48 kHz</option>
                <option value={88200}>88.2 kHz</option>
                <option value={96000}>96 kHz</option>
              </select>
            </div>

            <div className="setting-row">
              <label>Buffer Size</label>
              <select
                value={bufferSize}
                onChange={(e) => handleBufferSizeChange(Number(e.target.value))}
              >
                <option value={128}>128 samples</option>
                <option value={256}>256 samples</option>
                <option value={512}>512 samples</option>
                <option value={1024}>1024 samples</option>
                <option value={2048}>2048 samples</option>
              </select>
            </div>
          </div>
        )}

        {activeTab === 'midi' && (
          <div className="tab-content">
            <h2>MIDI & Sync Settings</h2>
            <p>MIDI configuration options...</p>
          </div>
        )}

        {activeTab === 'library' && (
          <div className="tab-content">
            <h2>Library & VST Settings</h2>

            <div className="setting-row">
              <label>VST3 Search Paths</label>
              <div className="path-list">
                {vst3Paths.map((path, index) => (
                  <div key={index} className="path-item">
                    <span>{path}</span>
                    <button onClick={() => handleRemoveVST3Path(index)}>
                      Remove
                    </button>
                  </div>
                ))}
                <button onClick={() => handleAddVST3Path('/Library/Audio/Plug-Ins/VST3')}>
                  Add Path
                </button>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default SettingsPanel;
