// api/WebSocketClient.ts

export class WebSocketClient {
    private ws: WebSocket | null = null;
    private url: string;

    constructor(url: string = "ws://localhost:50001") {
        this.url = url;
    }

    connect() {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
            console.log("Connected to FlowZone Engine");
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            // Handle state updates
        };
    }

    send(command: any) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(command));
        }
    }
}
