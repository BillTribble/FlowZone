const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:50001');

ws.on('open', function open() {
    console.log('connected');
    ws.send('hello');
});

ws.on('message', function incoming(data) {
    console.log('received: %s', data);
    if (data == 'Hello from FlowZone Engine') {
        console.log('✅ Handshake Verified');
        process.exit(0);
    }
});

setTimeout(() => {
    console.log('❌ Timeout waiting for handshake');
    process.exit(1);
}, 2000);
