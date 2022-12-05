/**
 * This code should send a network request when an orgasm has been denied. However, it is NOT tested!
 * This is just an example of what would be done, but I forgot the exact values and readings coming back from the EoM.
 * 
 * Essentially, when running the EoM in run/stop mode you can listen for the readings broadcasts and wait until
 * the motor speed goes to 0. A future firmware revision will also broadcast a denial event.
 * 
 * You will need to install ws and node-fetch:
 * 
 * npm install ws node-fetch
 */

import WebSocket, { WebSocketServer } from 'ws';
import fetch from 'node-fetch';

const EOM_IP = '192.168.1.100';
const REQUEST_URL = "http://192.168.1.200/relay/0?turn=on&timer=10";

const ws = new WebSocket(`ws://${EOM_IP}/`);

let previous_motor_speed = 0;

function handle_orgasm_denial() {
    // Your code here?
    fetch(REQUEST_URL)
        .then(console.log)
        .catch(console.error);
}

function readings(stats) {
    if (stats.motor_speed == 0 && previous_motor_speed >= 0) {
        handle_orgasm_denial();
    }

    previous_motor_speed = stats.motor_speed;
}

ws.on('open', () => {
    console.log("Connected to EOM3K");
});

ws.on('message', (data) => {
    console.log("Received: %s", data);
    const payload = JSON.parse(data);

    if (typeof payload.readings !== 'undefined') {
        readings(payload.readings);
    }
})