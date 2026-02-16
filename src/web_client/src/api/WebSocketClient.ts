// api/WebSocketClient.ts

export class WebSocketClient {
    private ws: WebSocket | null = null;
    private url: string;
    private onStateChange?: (state: any) => void;

    private reconnectDelay = 1000;
    private maxReconnectDelay = 30000;
    // private isConnected = false; // Unused for now

    constructor(url: string = "ws://localhost:50001") {
        this.url = url;
    }

    connect(onStateChange?: (state: any) => void) {
        this.onStateChange = onStateChange;
        this.initSocket();
    }

    private initSocket() {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
            console.log("[WebSocket] ✅ Connected to FlowZone Engine at", this.url);
            // this.isConnected = true;
            this.reconnectDelay = 1000; // Reset delay

            // Send reconnection info if implemented
            // this.send({ cmd: 'WS_RECONNECT', ... });
        };

        this.ws.onclose = () => {
            console.warn("[WebSocket] ⚠️ Disconnected. Reconnecting in " + this.reconnectDelay + "ms");
            // this.isConnected = false;
            setTimeout(() => {
                this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
                this.initSocket();
            }, this.reconnectDelay);
        };

        this.ws.onmessage = (event) => {
            console.log("[WebSocket] 📥 Message received:", event.data.substring(0, 200) + (event.data.length > 200 ? '...' : ''));
            try {
                const data = JSON.parse(event.data);
                console.log("[WebSocket] Parsed state keys:", Object.keys(data));
                if (this.onStateChange) {
                    this.onStateChange(data);
                }
            } catch (err) {
                console.error("[WebSocket] Failed to parse message:", err);
            }
        };

        this.ws.onerror = (error) => {
            console.error("[WebSocket] ❌ Error:", error);
        };
    }

    send(command: any) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            console.log("[WebSocket] 📤 Sending command:", command);
            this.ws.send(JSON.stringify(command));
        } else {
            console.warn("[WebSocket] ⚠️ Cannot send command - socket not open. State:", this.ws?.readyState);
        }
    }
}
