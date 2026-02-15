// api/WebSocketClient.ts

export class WebSocketClient {
    private ws: WebSocket | null = null;
    private url: string;
    private onStateChange?: (state: any) => void;

    constructor(url: string = "ws://localhost:50001") {
        this.url = url;
    }

    private reconnectDelay = 1000;
    private maxReconnectDelay = 30000;
    private isConnected = false;

    connect(onStateChange?: (state: any) => void) {
        this.onStateChange = onStateChange;
        this.initSocket();
    }

    private initSocket() {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
            console.log("Connected to FlowZone Engine");
            this.isConnected = true;
            this.reconnectDelay = 1000; // Reset delay

            // Send reconnection info if implemented
            // this.send({ cmd: 'WS_RECONNECT', ... });
        };

        this.ws.onclose = () => {
            console.log("Disconnected. Reconnecting in " + this.reconnectDelay + "ms");
            this.isConnected = false;
            setTimeout(() => {
                this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
                this.initSocket();
            }, this.reconnectDelay);
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if (this.onStateChange) {
                this.onStateChange(data);
            }
        };
    }

    send(command: any) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(command));
        }
    }
}
