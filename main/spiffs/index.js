const gateway = `ws://${window.location.hostname}/ws`;
let websocket;
let today = new Date(0);

window.addEventListener('load', () => {
    initWebSocket();
    startClock();
});

function initWebSocket() {
    console.log("Trying to open a WebSocket connection...");
    websocket = new WebSocket(gateway);

    websocket.onopen = () => {
        console.log("WebSocket connection opened.");
        document.getElementById("button").disabled = false;
        document.getElementById("button").addEventListener("click", toggle);
    };

    websocket.onclose = () => {
        console.log("WebSocket connection closed. Retrying...");
        document.getElementById("button").disabled = true;
        setTimeout(initWebSocket, 2000);
    };

    websocket.onerror = (event) => {
        console.error("WebSocket error:", event);
    };

    websocket.onmessage = (event) => {
        const msg = event.data;
        console.log("Received:", msg);

        if (msg === "T1") {
            document.getElementById("state").innerText = "State: ON";
        } else if (msg === "T0") {
            document.getElementById("state").innerText = "State: OFF";
        } else if (msg.startsWith("time:")) {
            const parsed = Date.parse("1970-01-01T" + msg.slice(5) + "Z");
            if (!isNaN(parsed)) today.setTime(parsed);
        }
    };
}

function toggle() {
    if (websocket.readyState === WebSocket.OPEN) {
        const d = new Date();
        websocket.send("timeoff:" + (d.getTimezoneOffset() * 60));
        websocket.send("time:" + d.getTime());
        websocket.send("toggle");
    } else {
        console.warn("WebSocket not ready. State:", websocket.readyState);
    }
}

function startClock() {
    function updateClock() {
        let h = today.getHours().toString().padStart(2, '0');
        let m = today.getMinutes().toString().padStart(2, '0');
        let s = today.getSeconds().toString().padStart(2, '0');
        document.getElementById("clock").innerText = `${h}:${m}:${s}`;
        today = new Date(today.valueOf() + 1000);
        setTimeout(updateClock, 1000);
    }
    updateClock();
}
