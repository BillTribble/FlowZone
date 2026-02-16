# FlowZone Human Testing Plan

## Pre-Build Verification

### Environment Check
- [ ] macOS version: Sequoia or later
- [ ] Xcode installed at `/Applications/Xcode.app`
- [ ] Developer directory configured: `xcode-select -p`
- [ ] JUCE 8.0.x available in `libs/`
- [ ] Node.js installed for React web client

### Build Verification
- [ ] Clean build successful (no errors)
- [ ] All test targets compile
- [ ] Standalone app launches
- [ ] No missing dependencies

## Core Audio Engine Tests

### Transport & Timing
1. **Start/Stop Transport**
   - [ ] Play button starts audio engine
   - [ ] Stop button halts playback cleanly
   - [ ] No audio glitches on start/stop
   - [ ] Metronome click audible when enabled

2. **BPM & Loop Length**
   - [ ] BPM changes reflected immediately
   - [ ] Loop length (1-8 bars) works correctly
   - [ ] Bar phase indicator moves smoothly
   - [ ] Ableton Link sync (if available)

3. **Audio I/O**
   - [ ] Input monitoring works
   - [ ] Output routing correct
   - [ ] No buffer underruns in Console
   - [ ] Latency acceptable (<10ms)

## Recording & Playback

### RetrospectiveBuffer
1. **Continuous Recording**
   - [ ] Always recording when transport active
   - [ ] Buffer holds 60 seconds minimum
   - [ ] CPU usage <5% during recording
   - [ ] No dropouts

2. **Retrospective Capture**
   - [ ] CAPTURE_RETROSPECTIVE command works
   - [ ] Audio captured correctly
   - [ ] Timestamp alignment accurate
   - [ ] Multiple captures don't interfere

### DiskWriter (4-Tier System)
1. **Tier 1: Normal Operation**
   - [ ] Records to WAV successfully
   - [ ] Files playable in external app
   - [ ] No corruption on normal stop
   - [ ] Buffer status normal (<80%)

2. **Tier 2: Warning**
   - [ ] Warning triggered at >80% buffer fill
   - [ ] UI shows warning badge
   - [ ] Audio continues recording
   - [ ] Log shows tier transition

3. **Tier 3: Overflow**
   - [ ] RAM blocks allocated when buffer full
   - [ ] Overflow size displayed correctly
   - [ ] Max 1GB overflow enforced
   - [ ] Flushes to disk when possible

4. **Tier 4: Critical**
   - [ ] Emergency FLAC created at >1GB
   - [ ] Recording stops gracefully
   - [ ] Error displayed to user
   - [ ] Partial audio saved

### Test Scenario: Slow Disk
- Simulate by recording to network drive
- Verify tiers trigger correctly
- Confirm audio playback never stops

## Crash Recovery (CrashGuard)

### Level 1: Plugin Crash Loop
1. **Setup**: Load known-crashy VST 3x in 60s
   - [ ] CrashGuard detects pattern
   - [ ] VSTs disabled automatically
   - [ ] Safe mode UI shown
   - [ ] Sentinel file created

2. **Recovery**:
   - [ ] App continues without VSTs
   - [ ] Clear crash history works
   - [ ] VSTs re-enabled after clear
   - [ ] Log shows all transitions

### Level 2: Audio Driver Failure
1. **Setup**: Disconnect audio interface mid-session
   - [ ] Driver failure detected
   - [ ] Audio device reset triggered
   - [ ] Safe mode Level 2 shown
   - [ ] Fallback to built-in works

### Level 3: Repeated Crashes
1. **Setup**: Force 3+ crashes
   - [ ] Factory defaults offered
   - [ ] Legacy config archived
   - [ ] Minimum safe config loads
   - [ ] App remains stable

### Sentinel File Lifecycle
- [ ] Created on startup
- [ ] Deleted on clean exit
- [ ] Persists after crash
- [ ] Cleared after 60s stable

## Configuration Management

### Config Loading
1. **Normal Load**
   - [ ] config.json loads correctly
   - [ ] All settings applied
   - [ ] No errors in log

2. **Backup Fallback**
   - [ ] Corrupt config → backup loads
   - [ ] Warning logged
   - [ ] Backup created on save

3. **Factory Defaults**
   - [ ] Both files corrupt → factory defaults
   - [ ] 44.1kHz, 512 buffer, no plugins
   - [ ] Legacy config archived

4. **Migration v0→v1**
   - [ ] Old config detected
   - [ ] Migration runs automatically
   - [ ] configVersion field added
   - [ ] Migration logged

### Minimum Safe Config
- [ ] Loads when everything fails
- [ ] 44.1kHz sample rate
- [ ] 512 buffer size
- [ ] Plugins disabled
- [ ] App starts successfully

## WebSocket Communication

### Connection
- [ ] WebSocket connects on startup (port 50001)
- [ ] Initial STATE_FULL received
- [ ] Revision ID increments
- [ ] No connection drops

### Commands
Test each command type:
- [ ] TRANSPORT_PLAY
- [ ] TRANSPORT_STOP
- [ ] SET_BPM
- [ ] SET_LOOP_LENGTH
- [ ] CAPTURE_RETROSPECTIVE
- [ ] RECORD_SLOT
- [ ] TRIGGER_SLOT

### State Updates
- [ ] STATE_DELTA updates received
- [ ] UI reflects state changes
- [ ] No missed updates
- [ ] Timestamp ordering correct

## React Web Client

### Settings Panel (4 Tabs)
1. **Interface Tab**
   - [ ] Theme toggle works
   - [ ] CSS variables update
   - [ ] Riff swap mode changes

2. **Audio Tab**
   - [ ] Device dropdown populated
   - [ ] Sample rate changes
   - [ ] Buffer size changes
   - [ ] Commands sent to engine

3. **MIDI & Sync Tab**
   - [ ] Tab renders
   - [ ] Placeholder content shown

4. **Library & VST Tab**
   - [ ] VST3 paths listed
   - [ ] Add path works
   - [ ] Remove path works
   - [ ] Changes persist

### Safe Mode Dialog
- [ ] Shows on crash detection
- [ ] Level indicator correct (1, 2, or 3)
- [ ] Description accurate
- [ ] Clear history button works
-[ ] Continue anyway works
- [ ] Factory reset works (Level 3)

## Performance Tests

### CPU Usage
- [ ] Idle: <2%
- [ ] Playing (no slots): <5%
- [ ] Playing (8 slots): <15%
- [ ] Recording: <20%
- [ ] Peak usage logged

### Memory
- [ ] Initial load: <100MB
- [ ] After 1 hour: <500MB
- [ ] No memory leaks visible
- [ ] Retrospective buffer stable

### Latency
- [ ] Round-trip <10ms @ 512 buffer
- [ ] Round-trip <5ms @ 256 buffer
- [ ] Jitter <1ms
- [ ] Stable over time

## Stress Testing

### Long Session
- [ ] Run 8+ hours
- [ ] No crashes
- [ ] No performance degradation
- [ ] Memory stable
- [ ] Logs clean

### Rapid Commands
- [ ] 100 BPM changes quickly
- [ ] 50 slot triggers rapidly
- [ ] Start/stop 100x
- [ ] No crashes or hangs
- [ ] Commands processed correctly

### Disk Stress
- [ ] Record 1GB+ continuously
- [ ] Multiple simultaneous recordings
- [ ] Network drive recording
- [ ] Tier system handles correctly

## Integration Tests

### Ableton Link (if available)
- [ ] Link peer discovered
- [ ] BPM sync works
- [ ] Transport sync works
- [ ] Quantum correct

### MIDI (future)
- [ ] Clock sync
- [ ] Control changes
- [ ] Note triggers

### VST3 Plugins (future)
- [ ] Scan paths
- [ ] Load plugins
- [ ] Process audio
- [ ] Crash recovery

## Logging & Diagnostics

### Log Files
- [ ] CrashGuard_Log.txt created
- [ ] ConfigManager_Log.txt created
- [ ] Timestamps ISO 8601 format
- [ ] All state transitions logged
- [ ] No sensitive data logged

### Console Output
- [ ] DBG statements visible
- [ ] Error messages clear
- [ ] Warning messages useful
- [ ] No spam

## Edge Cases

### File System
- [ ] Read-only config directory
- [ ] Disk full during recording
- [ ] Network drive disconnect
- [ ] Permissions errors

### Audio Devices
- [ ] No audio device available
- [ ] Device disconnected mid-session
- [ ] Device sample rate change
- [ ] Multiple device switches

### Timing
- [ ] System time change
- [ ] Sleep/wake cycle
- [ ] CPU throttling
- [ ] Background app interference

## Regression Prevention

Run after each build:
- [ ] All Catch2 tests pass
- [ ] All Vitest tests pass
- [ ] No new compiler warnings
- [ ] No memory leaks (Instruments)
- [ ] No hanging threads

## Sign-Off Checklist

Before declaring "ready for production":
- [ ] All critical tests passed
- [ ] No known crashes
- [ ] Performance acceptable
- [ ] Logs clean
- [ ] Documentation complete
- [ ] Recovery mechanisms tested
- [ ] User can recover from all error states
- [ ] No data loss scenarios

## Test Results Recording

For each test session, record:
- Date/time
- macOS version
- Build configuration (Debug/Release)
- Issues found
- Steps to reproduce
- Expected vs actual behavior
- Severity (Critical/High/Medium/Low)

## Known Issues Template

```
Issue ID: TST-XXX
Title: [Brief description]
Severity: [Critical/High/Medium/Low]
Steps to Reproduce:
1. 
2. 
3. 
Expected: [What should happen]
Actual: [What actually happens]
Workaround: [If any]
Status: [Open/Fixed/Deferred]
```
