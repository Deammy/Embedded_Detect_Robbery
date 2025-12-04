#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "ServerName";
const char* WIFI_PASSWORD = "ServerPassword";

WebServer server(80);

void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32-S3 Web Server</title>
    <style>
      body { 
        background-color: #121212; 
        color: white; 
        font-family: Arial, sans-serif; 
        text-align: center;
        padding-top: 50px;
      }
      .box {
        background:#1e1e1e;
        display:inline-block;
        padding:20px;
        border-radius:10px;
        box-shadow:0 0 10px rgba(255,255,255,0.2);
      }
      h1 { color:#00eaff; }
      button {
        padding:10px 20px;
        font-size:16px;
        border:none;
        border-radius:5px;
        background:#00eaff;
        cursor:pointer;
      }
      button:hover {
        background:#0099bb;
      }
    </style>
  </head>
  <body>
    <div class="box">
      <h1>Hello from ESP32-S3</h1>
      <p>This is a webpage hosted directly from your microcontroller 👋</p>
      <button onclick="alert('ESP32-S3 says hi!')">Click Me</button>
    </div>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void setupWiFi() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  setupWiFi();
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
}
