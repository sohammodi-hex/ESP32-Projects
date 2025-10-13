/*
  ESP8266 WiFi Car (AP Web UI)
  - L298N drive only (no MOSFET / no auxiliary motor control)
  - AP mode: connect to CAR_WIFI and open http://192.168.4.1
  - Board: NodeMCU 1.0 or Wemos D1 mini (ESP8266)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ====== USER SETTINGS ======
const char* AP_SSID = "CAR_WIFI";
const char* AP_PASS = "12345678";

// Pin choices: prefer D1, D2, D5, D6, D7 for outputs
const uint8_t IN1 = D1; // GPIO5
const uint8_t IN2 = D2; // GPIO4
const uint8_t IN3 = D3; // GPIO12
const uint8_t IN4 = D4; // GPIO13

// Optional PWM enables (set to pin or -1 if jumpers on L298N)
const int ENA = -1;  // set to D5/D6/D7 if needed, else -1
const int ENB = -1;  // set to D5/D6/D7 if needed
const int DEFAULT_SPEED = 200; // ESP8266 analogWrite defaults to 0..255

ESP8266WebServer server(80);

// ====== motion watchdog ======
volatile uint32_t lastMotionMs = 0;
volatile bool moving = false;
const uint32_t MOTOR_TIMEOUT_MS = 1500;

// ====== HTML UI (no Motor toggle button) ======
const char PAGE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>WiFi Car</title>
<style>
  body { font-family: Arial, sans-serif; text-align:center; max-width:420px; margin:20px auto; }
  h1 { font-size: 26px; }
  .grid { display:grid; grid-template-columns: 1fr 1fr 1fr; gap:12px; margin:20px 0; }
  button { font-size:20px; padding:14px; border-radius:8px; border:1px solid #333; transition: transform .05s ease, filter .12s; touch-action:none; }
  button:active, .pressed { transform: scale(0.98); filter: brightness(0.94); }
  .wide { grid-column: 1 / 4; }
  .secondary { background:#f2f2f2; }
  .primary { background:#e6f3ff; }
  .status { margin-top:14px; font-size:14px; color:#333; display:flex; justify-content:center; align-items:center; gap:8px; }
  .dot { width:10px; height:10px; border-radius:50%; background:#bbb; display:inline-block; }
  .dot.ok { background:#2ecc71; }
  .dot.fail { background:#e74c3c; }
  #last { font-weight:600; }
</style>
<script>
let keepaliveTimer = null;
async function call(cmd){
  try { return await fetch(cmd, {cache:'no-store'}); }
  catch(e) { return { ok:false }; }
}
function startHold(btn, cmd, label){
  if(btn){ btn.classList.add('pressed'); }
  call(cmd).then(r=>{ document.getElementById('last').textContent = label + (r.ok?" ✓":" ✕"); });
  keepaliveTimer = setInterval(()=>{ call(cmd); }, 250);
}
function endHold(btn){
  if(btn){ btn.classList.remove('pressed'); }
  call('/stop');
  if(keepaliveTimer){ clearInterval(keepaliveTimer); keepaliveTimer = null; }
}
function bindHold(id, cmd, label){
  const el = document.getElementById(id);
  if(!el) return;
  const down = (e)=>{ e.preventDefault(); startHold(el, cmd, label); };
  const up   = (e)=>{ e.preventDefault(); endHold(el); };
  el.addEventListener('pointerdown', down);
  el.addEventListener('pointerup',   up);
  el.addEventListener('pointercancel', up);
  el.addEventListener('pointerleave', up);
}
async function clickOnce(cmd, label){
  const r = await call(cmd);
  document.getElementById('last').textContent = label + (r.ok?" ✓":" ✕");
}
async function ping(){
  const dot = document.getElementById('dot');
  const r = await call('/ping');
  if(r.ok){ dot.classList.add('ok'); dot.classList.remove('fail'); }
  else { dot.classList.add('fail'); dot.classList.remove('ok'); }
}
setInterval(ping, 1000);
document.addEventListener('visibilitychange', ()=>{
  if(document.hidden){ endHold(document.querySelector('.pressed')); }
});
window.addEventListener('pagehide', ()=>{ endHold(document.querySelector('.pressed')); });
window.addEventListener('load', ()=>{
  ping();
  bindHold('btnF','/forward','Forward');
  bindHold('btnL','/left','Left');
  bindHold('btnR','/right','Right');
  bindHold('btnB','/backward','Backward');
});
</script>
</head>
<body>
  <h1>WiFi Car Controller</h1>
  <div class="grid">
    <button id="btnF" class="wide primary">Forward</button>
    <button id="btnL" class="secondary">Left</button>
    <button id="btnS" class="secondary" onclick="clickOnce('/stop','Stop')">Stop</button>
    <button id="btnR" class="secondary">Right</button>
    <button id="btnB" class="wide primary">Backward</button>
  </div>
  <div class="status">
    <span id="dot" class="dot"></span>
    <span>Device:</span>
    <span id="last">Ready</span>
  </div>
  <p>Connect to AP: CAR_WIFI, open http://192.168.4.1</p>
</body>
</html>
)HTML";

// ====== MOTOR HELPERS (car drive) ======
void motorsStop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  moving = false;
}
void motorsForward(){ digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);  moving = true; }
void motorsBackward(){digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);   digitalWrite(IN4, HIGH); moving = true; }
void motorsLeft()    {digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);  moving = true; }
void motorsRight()   {digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);   digitalWrite(IN4, HIGH); moving = true; }

void setDefaultSpeed(){
  // If ENA/ENB are wired to ESP8266 pins and jumpers removed on L298N, enable PWM here.
  if (ENA >= 0) analogWrite(ENA, DEFAULT_SPEED);
  if (ENB >= 0) analogWrite(ENB, DEFAULT_SPEED);
}

// ====== HTTP HANDLERS ======
void handleRoot(){ server.send_P(200, "text/html", PAGE_HTML); }
inline void noteMotion(){ lastMotionMs = millis(); moving = true; }
void handleForward(){ setDefaultSpeed(); motorsForward(); noteMotion(); server.send(200,"text/plain","OK"); }
void handleBackward(){ setDefaultSpeed(); motorsBackward(); noteMotion(); server.send(200,"text/plain","OK"); }
void handleLeft()    { setDefaultSpeed(); motorsLeft();     noteMotion(); server.send(200,"text/plain","OK"); }
void handleRight()   { setDefaultSpeed(); motorsRight();    noteMotion(); server.send(200,"text/plain","OK"); }
void handleStop()    { motorsStop(); server.send(200,"text/plain","OK"); }
void handlePing(){ server.send(200, "text/plain", "OK"); }

void setup(){
  // Pin modes
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  if (ENA >= 0) pinMode(ENA, OUTPUT);
  if (ENB >= 0) pinMode(ENB, OUTPUT);

  motorsStop();

  // Wi‑Fi AP + WebServer routes
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS); // default AP IP: 192.168.4.1
  server.on("/",        HTTP_GET, handleRoot);
  server.on("/forward", HTTP_GET, handleForward);
  server.on("/backward",HTTP_GET, handleBackward);
  server.on("/left",    HTTP_GET, handleLeft);
  server.on("/right",   HTTP_GET, handleRight);
  server.on("/stop",    HTTP_GET, handleStop);
  server.on("/ping",    HTTP_GET, handlePing);
  server.begin();

  lastMotionMs = millis();
}

void loop(){
  server.handleClient();
  if (moving && (millis() - lastMotionMs > MOTOR_TIMEOUT_MS)) {
    motorsStop();
  }
}
