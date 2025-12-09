#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// ================= WIFI ==================
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// ================= ThingSpeak ==================
const String THINGSPEAK_API_KEY = "";

WebServer server(80);

String lastMsg = "Waiting...";
String serialBuffer = "";
unsigned long lastSerialTime = 0;
unsigned long ts_lastUpload = 0;     // ← anti-spam upload limiter

// =============================================================
// SEND DATA TO THINGSPEAK
// =============================================================
void uploadThingSpeak(int field, int value, String msg){
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - ts_lastUpload < 1500) return;   // ← limit spam (ThingSpeak rule)

  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + THINGSPEAK_API_KEY;

  if(field == 1) url += "&field1=" + String(value);    // LASER
  if(field == 2) url += "&field2=" + String(value);    // TILT
  if(field == 3) url += "&field3=" + String(value);    // OPEN CLOSE state

  msg.replace(" ", "%20");
  url += "&status=" + msg;

  http.begin(url);
  http.GET();
  http.end();

  ts_lastUpload = millis();
}


// ================= WEBPAGE =================
void handleRoot(){
  String page = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
  <title>Security Panel</title>

  <style>
    body{margin:0;background:#0a0f16;font-family:Segoe UI,Arial;color:#e9f1ff;
         display:flex;justify-content:center;align-items:center;height:100vh;}
    .card{background:rgba(255,255,255,0.05);padding:35px 50px;border-radius:15px;
          width:420px;text-align:center;box-shadow:0 0 25px rgba(0,255,255,0.25);
          backdrop-filter: blur(6px);}
    h2{margin-top:-5px;font-size:30px;color:#7de0ff;}
    button{width:45%;margin:10px;font-size:22px;padding:14px 0;border:none;
           border-radius:8px;font-weight:bold;cursor:pointer;transition:0.25s;}
    .openBtn{background:#18ff72;} .openBtn:hover{box-shadow:0 0 15px #18ff72;}
    .closeBtn{background:#ff3b3b;} .closeBtn:hover{box-shadow:0 0 15px #ff3b3b;}

    #statusBox{margin-top:20px;font-size:26px;font-weight:600;color:#00e5ff;
               background:#001722;padding:10px;border-radius:8px;
               box-shadow:0 0 10px #00e5ff inset;}

    #alertBox{display:none;margin-top:15px;padding:10px;border-radius:8px;
              font-size:22px;font-weight:bold;color:white;
              animation:blink 0.7s infinite alternate;}
    @keyframes blink{from{opacity:.4;} to{opacity:1;}}
  </style>
  </head>

  <body>
    <div class="card">
      <h2>Security Control</h2>

      <button class="openBtn" onclick="fetch('/cmd/open')">OPEN (1)</button>
      <button class="closeBtn" onclick="fetch('/cmd/close')">CLOSE (0)</button>

      <div id="statusBox">Waiting...</div>
      <div id="alertBox">ALERT TRIGGERED</div>
      <audio id="beep" src="https://actions.google.com/sounds/v1/alarms/beep_short.ogg"></audio>
    </div>

<script>
setInterval(()=>{
  fetch('/status').then(r=>r.text()).then(data=>{

    const laser = data.toLowerCase().includes("laser");
    const tilt  = data.toLowerCase().includes("tilt");

    let alertBox=document.getElementById("alertBox");
    let statusBox=document.getElementById("statusBox");
    let beep = document.getElementById("beep");

    // NORMAL STATE
    if(!laser && !tilt){
      alertBox.style.display="none";
      statusBox.innerHTML="Normal";
      return;
    }

    // ALERT STATE
    alertBox.style.display="block";
    statusBox.innerHTML = data;
    beep.play();

    if(laser && tilt)       alertBox.innerHTML="LASER + TILT ALERT";
    else if(laser)          alertBox.innerHTML="LASER ALERT";
    else if(tilt)           alertBox.innerHTML="TILT ALERT";
  });
},400);
</script>

</body></html>
)=====";

  server.send(200,"text/html",page);
}


// ================== SETUP ====================
void setup(){
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 17, 18); // RX,TX

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status()!=WL_CONNECTED){ Serial.print("."); delay(200); }
  Serial.println("\nWiFi Connected → " + WiFi.localIP().toString());

  server.on("/",handleRoot);
  server.on("/status",[]{ server.send(200,"text/plain",lastMsg); });

  server.on("/cmd/open", [](){
    Serial1.println("1");
    lastMsg="OPEN";
    uploadThingSpeak(3,1,"OPEN");
    server.send(200,"text/plain","OPENED");
  });

  server.on("/cmd/close", [](){
    Serial1.println("0");
    lastMsg="CLOSE";
    uploadThingSpeak(3,0,"CLOSE");
    server.send(200,"text/plain","CLOSED");
  });

  server.begin();
  Serial.println("WEB Server Ready");
}


// ================= MAIN LOOP =================
void loop(){
  server.handleClient();

  // UART input from STM32
  while(Serial1.available()){
    char c = Serial1.read();
    if(c=='\n'){                // ← now handles \r \n or both
      if(serialBuffer.length()>0){
        lastMsg = serialBuffer;

        // auto upload
        if(lastMsg.indexOf("laser")>=0) uploadThingSpeak(1,1,lastMsg);
        if(lastMsg.indexOf("tilt") >=0) uploadThingSpeak(2,1,lastMsg);

        Serial.println("STM32 → " + lastMsg);
        lastSerialTime = millis();
      }
      serialBuffer="";
      break;
    }
    else serialBuffer += c;
  }
  if(millis()-lastSerialTime > 2000){
      lastMsg = "Normal";
    }
}
