var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var webtime = 0;
var today = new Date(0);
window.addEventListener('load', onLoad);
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}
function onOpen(event) {
    console.log('Connection opened');
    websocket.send("update");
}
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    var state;
    console.log(event.data);
    let msg = event.data;
    if (msg == "T1") {
        state = "ON";
        document.getElementById('state').innerHTML = state;
    }
    else if (msg == "T0") {
        state = "OFF";
        document.getElementById('state').innerHTML = state;
    }
    if (msg.includes('time:')) {
        today.setTime(Number.parseInt(msg.substr(5)));
    }
    /*state = event.data;*/
    //            document.getElementById('state').innerHTML = state;
}
function onLoad(event) {
    initWebSocket();
    initButton();
    startTime();
}
function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
}
function toggle() {
    const d = new Date();
    let jtime = "timeoff:" + d.getTimezoneOffset();
    websocket.send(jtime);
    jtime = "time:" + d.getTime()
    websocket.send(jtime);
    websocket.send("toggle");
}

function startTime() {
    let h = today.getHours();
    let m = today.getMinutes();
    let s = today.getSeconds();
    h = checkTime(h);
    m = checkTime(m);
    s = checkTime(s);
    document.getElementById('clock').innerHTML = h + ":" + m + ":" + s;
    //            document.getElementById('t_z').innerHTML = today.getTimezoneOffset();
    today = new Date(today.valueOf() + 500);
    setTimeout(startTime, 500);
}

function checkTime(i) {
    if (i < 10) { i = "0" + i };  // add zero in front of numbers < 10
    return i;
}
