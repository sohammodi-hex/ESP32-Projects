/*
  ESP8266 WiFi Forklift Car (AP Web UI + Load Cell + Web Tare + Serial Display)
  - Controls movement + forklift lift + weight measurement
  - Weight auto-zeroes at startup
  - Serial monitor shows live weight
  - Web page has "Tare (Zero)" button for resetting weight
  - Connect to WiFi: CAR_WIFI | URL: http://192.168.4.1
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <HX711.h>

// ====== WiFi Config ======
const char* AP_SSID = "CAR_WIFI";
const char* AP_PASS = "12345678";

// ====== Movement Motor Pins ======
const uint8_t IN1 = D1;
const uint8_t IN2 = D2;
const uint8_t IN3 = D3;
const uint8_t IN4 = D4;

// ====== Forklift Lift Motor Pins ======
const uint8_t LIFT_IN1 = D5;
const uint8_t LIFT_IN2 = D6;

// ====== Load Cell (HX711) ======
#define HX711_DOUT D7
#define HX711_SCK  D8
HX711 scale;
float calibration_factor = -7050.0;  // Adjust as needed for your load cell

// ====== Motor Config ======
ESP8266WebServer server(80);
volatile uint32_t lastMotionMs = 0;
volatile bool moving = false;
const uint32_t MOTOR_TIMEOUT_MS = 1500;

// ====== HTML PAGE ======
const char PAGE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>WiFi Forklift Controller</title>
<style>
  body { font-family: Arial, sans-serif; text-align:center; max-width:420px; margin:20px auto; }
  h1 { font-size: 26px; }
  .grid { display:grid; grid-template-columns: 1fr 1fr 1fr; gap:12px; margin:20px 0; }
  button { font-size:20px; padding:14px; border-radius:8px; border:1px solid #333;
           transition: transform .05s ease, filter .12s; touch-action:none; }
  button:active, .pressed { transform: scale(0.98); filter: brightness(0.94); }
  .wide { grid-column: 1 / 4; }
  .secondary { background:#f2f2f2; }
  .primary { background:#e6f3ff; }
  .lift { background:#d4edda; }
  .tare { background:#fff3cd; }
  #weight { font-size:22px; font-weight:600; color:#222; margin-top:10px; }
</style>
<script>
let keepaliveTimer=null;
async function call(cmd){try{return await fetch(cmd,{cache:'no-store'});}catch(e){return {ok:false};}}
function startHold(btn,cmd){if(btn)btn.classList.add('pressed');call(cmd);keepaliveTimer=setInterval(()=>{call(cmd);},250);}
function endHold(btn){if(btn)btn.classList.remove('pressed');call('/stop');if(keepaliveTimer){clearInterval(keepaliveTimer);keepaliveTimer=null;}}
function bindHold(id,cmd){
  const el=document.getElementById(id);
  if(!el)return;
  const down=e=>{e.preventDefault();startHold(el,cmd);};
  const up=e=>{e.preventDefault();endHold(el);};
  el.addEventListener('pointerdown',down);
  el.addEventListener('pointerup',up);
  el.addEventListener('pointercancel',up);
  el.addEventListener('pointerleave',up);
}
async function updateWeight(){
  const res=await fetch('/weight');
  const w=await res.text();
  document.getElementById('weight').textContent="Weight: "+w+" kg";
}
setInterval(updateWeight,1000);
window.addEventListener('load',()=>{
  updateWeight();
  bindHold('btnF','/forward');
  bindHold('btnL','/left');
  bindHold('btnR','/right');
  bindHold('btnB','/backward');
  bindHold('btnUp','/liftup');
  bindHold('btnDn','/liftdown');
});
</script>
</head>
<body>
  <h1>WiFi Forklift Controller</h1>
  <div class="grid">
    <button id="btnF" class="wide primary">Forward</button>
    <button id="btnL" class="secondary">Left</button>
    <button id="btnS" class="secondary" onclick="fetch('/stop')">Stop</button>
    <button id="btnR" class="secondary">Right</button>
    <button id="btnB" class="wide primary">Backward</button>
    <button id="btnUp" class="lift wide">Lift Up</button>
    <button id="btnDn" class="lift wide">Lift Down</button>
    <button class="tare wide" onclick="fetch('/tare')">⚖️ Tare (Zero)</button>
  </div>
  <div id="weight">Weight: 0.00 kg</div>
  <p>Connect to WiFi: <b>CAR_WIFI</b><br>Open <b>http://192.168.4.1</b></p>
</body>
</html>
)HTML";

// ====== MOTOR CONTROL ======
void motorsStop(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
  digitalWrite(LIFT_IN1,LOW); digitalWrite(LIFT_IN2,LOW);
  moving=false;
}
void motorsForward(){digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW);moving=true;}
void motorsBackward(){digitalWrite(IN1,LOW);digitalWrite(IN2,HIGH);digitalWrite(IN3,LOW);digitalWrite(IN4,HIGH);moving=true;}
void motorsLeft(){digitalWrite(IN1,LOW);digitalWrite(IN2,HIGH);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW);moving=true;}
void motorsRight(){digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW);digitalWrite(IN3,LOW);digitalWrite(IN4,HIGH);moving=true;}
void liftUp(){digitalWrite(LIFT_IN1,HIGH);digitalWrite(LIFT_IN2,LOW);moving=true;}
void liftDown(){digitalWrite(LIFT_IN1,LOW);digitalWrite(LIFT_IN2,HIGH);moving=true;}

// ====== HTTP HANDLERS ======
void handleRoot(){server.send_P(200,"text/html",PAGE_HTML);}
void handleForward(){motorsForward();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleBackward(){motorsBackward();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleLeft(){motorsLeft();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleRight(){motorsRight();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleStop(){motorsStop();server.send(200,"text/plain","OK");}
void handleLiftUp(){liftUp();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleLiftDown(){liftDown();lastMotionMs=millis();server.send(200,"text/plain","OK");}
void handleTare(){
  scale.tare();
  Serial.println("TARE executed via web button.");
  server.send(200,"text/plain","TARE OK");
}
void handleWeight(){
  float w = scale.get_units(5);
  String msg = String(w, 2);
  server.send(200,"text/plain",msg);
}

// ====== SETUP ======
void setup(){
  Serial.begin(9600);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(LIFT_IN1,OUTPUT); pinMode(LIFT_IN2,OUTPUT);
  motorsStop();

  // HX711 setup
  scale.begin(HX711_DOUT, HX711_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("System Ready — Weight zeroed at startup.");

  // WiFi setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("WiFi Access Point: CAR_WIFI");
  Serial.println("Open http://192.168.4.1 in browser");

  // Web routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/forward", HTTP_GET, handleForward);
  server.on("/backward", HTTP_GET, handleBackward);
  server.on("/left", HTTP_GET, handleLeft);
  server.on("/right", HTTP_GET, handleRight);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/liftup", HTTP_GET, handleLiftUp);
  server.on("/liftdown", HTTP_GET, handleLiftDown);
  server.on("/tare", HTTP_GET, handleTare);
  server.on("/weight", HTTP_GET, handleWeight);
  server.begin();

  lastMotionMs = millis();
}

// ====== LOOP ======
void loop(){
  server.handleClient();

  // Auto-stop motors after timeout
  if(moving && (millis() - lastMotionMs > MOTOR_TIMEOUT_MS)){
    motorsStop();
  }

  // Print weight on Serial Monitor every second
  static unsigned long lastPrint = 0;
  if(millis() - lastPrint > 1000){
    float w = scale.get_units(3);
    Serial.print("Current Weight: ");
    Serial.print(w, 2);
    Serial.println(" kg");
    lastPrint = millis();
  }
}
