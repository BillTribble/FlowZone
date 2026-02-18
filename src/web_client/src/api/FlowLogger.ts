// api/FlowLogger.ts
// Browser-based logger for FlowZone diagnostics.
// Stores entries in a ring buffer; downloadable as .log file via Ctrl+Shift+L.

type LogCategory = 'WS' | 'AUDIO' | 'STATE';

interface LogEntry {
    timestamp: string;
    category: LogCategory;
    message: string;
}

class FlowLogger {
    private entries: LogEntry[] = [];
    private maxEntries = 500;

    log(category: LogCategory, message: string) {
        const timestamp = new Date().toISOString();
        const entry = { timestamp, category, message };

        this.entries.push(entry);
        if (this.entries.length > this.maxEntries) {
            this.entries.shift();
        }

        console.log(`[FlowZone:${category}] ${message}`);
    }

    // Sample-based logging: only log every Nth call
    private counters: Record<string, number> = {};
    logSampled(category: LogCategory, key: string, message: string, interval: number) {
        const count = (this.counters[key] || 0) + 1;
        this.counters[key] = count;
        if (count >= interval) {
            this.counters[key] = 0;
            this.log(category, message);
        }
    }

    downloadLogs() {
        const content = this.entries
            .map(e => `${e.timestamp} [${e.category}] ${e.message}`)
            .join('\n');

        const blob = new Blob([content], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `flowzone_frontend_${new Date().toISOString().replace(/[:.]/g, '-')}.log`;
        a.click();
        URL.revokeObjectURL(url);
    }

    getEntries(): LogEntry[] {
        return [...this.entries];
    }
}

export const flowLogger = new FlowLogger();

// Install keyboard shortcut: Ctrl+Shift+L to download logs
if (typeof window !== 'undefined') {
    window.addEventListener('keydown', (e) => {
        if (e.ctrlKey && e.shiftKey && e.key === 'L') {
            e.preventDefault();
            flowLogger.downloadLogs();
        }
    });
}
