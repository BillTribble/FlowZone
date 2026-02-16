import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { SettingsPanel } from './SettingsPanel';

describe('SettingsPanel', () => {
  it('renders all 4 tabs', () => {
    render(<SettingsPanel onClose={() => { }} />);

    expect(screen.getByText('Interface')).toBeDefined();
    expect(screen.getByText('Audio')).toBeDefined();
    expect(screen.getByText('MIDI & Sync')).toBeDefined();
    expect(screen.getByText('Library & VST')).toBeDefined();
  });

  it('switches between tabs', () => {
    render(<SettingsPanel onClose={() => { }} />);

    const audioTab = screen.getByText('Audio');
    fireEvent.click(audioTab);

    expect(screen.getByText('Audio Settings')).toBeDefined();
  });

  it('theme toggle changes data-theme attribute', () => {
    render(<SettingsPanel onClose={() => { }} />);

    const themeButton = screen.getByText(/Toggle Theme/);
    fireEvent.click(themeButton);

    expect(document.documentElement.getAttribute('data-theme')).toBe('light');
  });

  it('audio device dropdown triggers SET_AUDIO_DEVICE', () => {
    const consoleSpy = vi.spyOn(console, 'log');
    render(<SettingsPanel onClose={() => { }} />);

    const audioTab = screen.getByText('Audio');
    fireEvent.click(audioTab);

    const deviceSelect = screen.getByLabelText('Audio Device') as HTMLSelectElement;
    fireEvent.change(deviceSelect, { target: { value: 'CoreAudio' } });

    expect(consoleSpy).toHaveBeenCalledWith('SET_AUDIO_DEVICE:', 'CoreAudio');
  });

  it('sample rate dropdown triggers SET_SAMPLE_RATE', () => {
    const consoleSpy = vi.spyOn(console, 'log');
    render(<SettingsPanel onClose={() => { }} />);

    const audioTab = screen.getByText('Audio');
    fireEvent.click(audioTab);

    const sampleRateSelect = screen.getByLabelText('Sample Rate') as HTMLSelectElement;
    fireEvent.change(sampleRateSelect, { target: { value: '48000' } });

    expect(consoleSpy).toHaveBeenCalledWith('SET_SAMPLE_RATE:', 48000);
  });

  it('buffer size dropdown triggers SET_BUFFER_SIZE', () => {
    const consoleSpy = vi.spyOn(console, 'log');
    render(<SettingsPanel onClose={() => { }} />);

    const audioTab = screen.getByText('Audio');
    fireEvent.click(audioTab);

    const bufferSizeSelect = screen.getByLabelText('Buffer Size') as HTMLSelectElement;
    fireEvent.change(bufferSizeSelect, { target: { value: '256' } });

    expect(consoleSpy).toHaveBeenCalledWith('SET_BUFFER_SIZE:', 256);
  });

  it('VST3 search path add/remove works', () => {
    const consoleSpy = vi.spyOn(console, 'log');
    render(<SettingsPanel onClose={() => { }} />);

    const libraryTab = screen.getByText('Library & VST');
    fireEvent.click(libraryTab);

    const addButton = screen.getByText('Add Path');
    fireEvent.click(addButton);

    expect(consoleSpy).toHaveBeenCalledWith('Added VST3 path:', '/Library/Audio/Plug-Ins/VST3');

    const removeButton = screen.getByText('Remove');
    fireEvent.click(removeButton);

    expect(consoleSpy).toHaveBeenCalledWith('Removed VST3 path at index:', 0);
  });

  it('Riff Swap Mode radio buttons work', () => {
    const consoleSpy = vi.spyOn(console, 'log');
    render(<SettingsPanel onClose={() => { }} />);

    const immediateRadio = screen.getByLabelText('Immediate') as HTMLInputElement;
    fireEvent.click(immediateRadio);

    expect(consoleSpy).toHaveBeenCalledWith('Riff Swap Mode:', 'immediate');
    expect(immediateRadio.checked).toBe(true);
  });
});
