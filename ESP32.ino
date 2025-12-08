#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// ================= WIFI ==================
const char* WIFI_SSID = "CanITewYou Phone";
const char* WIFI_PASSWORD = "ajxr1535";

// ================= ThingSpeak ==================
const String THINGSPEAK_API_KEY = "JU0TUDBDU27AOERS";

WebServer server(80);

String lastMsg = "Waiting...";
String serialBuffer = "";
bool openState = false;   // 1 = open , 0 = close

// =============================================================
// SEND DATA TO THINGSPEAK
// =============================================================
void uploadThingSpeak(int field, int value, String msg){
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + THINGSPEAK_API_KEY;

  if(field == 1) url += "&field1=1";     // LASER
  if(field == 2) url += "&field2=1";     // TILT
  if(field == 3) url += "&field3=" + String(value);     // OPEN/CLOSE

  msg.replace(" ", "%20");
  url += "&status=" + msg;

  http.begin(url);
  http.GET();
  http.end();
}

// =============================================================
// WEBPAGE
// =============================================================
void handleRoot(){
  String page = R"html(
  <html>
  <head>
  <title>Security Panel</title>

  <style>
    body{
      margin:0;background:#0a0f16;font-family:Segoe UI,Arial;color:#e9f1ff;
      display:flex;justify-content:center;align-items:center;height:100vh;
    }
    .card{
      background:rgba(255,255,255,0.05);
      padding:35px 50px;border-radius:15px;width:420px;text-align:center;
      box-shadow:0 0 25px rgba(0,255,255,0.2);
      backdrop-filter: blur(8px);
    }
    h2{margin-top:-10px;font-size:30px;color:#7de0ff;text-shadow:0 0 10px #00eaff;}
    button{
      width:45%;margin:10px;font-size:23px;padding:14px 0;border:none;
      border-radius:8px;font-weight:bold;transition:.25s;cursor:pointer;
    }
    .openBtn{background:#18ff72;} .openBtn:hover{box-shadow:0 0 15px #18ff72;}
    .closeBtn{background:#ff3b3b;} .closeBtn:hover{box-shadow:0 0 15px #ff3b3b;}

    #statusBox{
      margin-top:25px;font-size:26px;font-weight:bold;
      color:#00e5ff;background:#001722;padding:10px 0;border-radius:8px;
      box-shadow:0 0 10px #00e5ff inset;
    }

    /* ALERT BANNER */
    #alertBox{
      display:none;margin-top:18px;padding:10px;border-radius:8px;
      font-size:22px;font-weight:bold;color:#fff;
      animation:blink 0.7s infinite alternate;
    }
    @keyframes blink { from{opacity:.4;} to{opacity:1;} }
  </style>
  </head>

  <body>
    <div class="card">
      <h2>Security Control</h2>

      <button class="openBtn"  onclick="fetch('/cmd/open')">OPEN (1)</button>
      <button class="closeBtn" onclick="fetch('/cmd/close')">CLOSE (0)</button>

      <div id="statusBox">Waiting...</div>
      <div id="alertBox">ALERT TRIGGERED</div>
      <audio id="beep" src="https://actions.google.com/sounds/v1/alarms/beep_short.ogg"></audio>
    </div>

  <script>
    let lastState="";

    setInterval(()=>{
      fetch('/status').then(r=>r.text()).then(data=>{
        document.getElementById("statusBox").innerHTML = data;

        // ============= ALERT SYSTEM =============
        if((data.includes("laser") || data.includes("tilt")) && data!==lastState){
          document.getElementById("alertBox").style.display="block";
          document.getElementById("beep").play();
        } else if(!(data.includes("laser")||data.includes("tilt"))){
          document.getElementById("alertBox").style.display="none";
        }

        lastState=data;
      });
    },400);
  </script>

  </body></html>
  )html";

  server.send(200,"text/html",page);
}

// =============================================================
// SETUP
// =============================================================
void setup(){
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 17,18); // RX=17 TX=18

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status()!=WL_CONNECTED){ Serial.print("."); delay(150); }
  Serial.println("\nWiFi OK  → " + WiFi.localIP().toString());

  server.on("/",handleRoot);
  server.on("/status",[]{ server.send(200,"text/plain",lastMsg); });

  // ---------- WEB COMMAND: OPEN ----------
  server.on("/cmd/open", [](){
    Serial1.println("1");
    lastMsg="OPEN";
    uploadThingSpeak(3,1,"OPEN");
    Serial.println(lastMsg);
    server.send(200,"text/plain","OPENED");
  });

  // ---------- WEB COMMAND: CLOSE ----------
  server.on("/cmd/close", [](){
    Serial1.println("0");
    lastMsg="CLOSE";
    uploadThingSpeak(3,0,"CLOSE");
    Serial.println(lastMsg);
    server.send(200,"text/plain","CLOSED");
  });

  server.begin();
  Serial.println("WEB Server Ready");
}

// =============================================================
// MAIN LOOP = READ STM32 DATA
// =============================================================
void loop(){
  server.handleClient();

  while(Serial1.available()){
    char c = Serial1.read();

    if(c=='\n'){ // ---- COMPLETE MESSAGE ----
      lastMsg = serialBuffer;

      if(serialBuffer.indexOf("laser")>=0) uploadThingSpeak(1,1,lastMsg);
      if(serialBuffer.indexOf("tilt") >=0) uploadThingSpeak(2,1,lastMsg);

      serialBuffer=""; // ← REAL BUFFER CLEAR
    }
    else serialBuffer += c;
  }
}
