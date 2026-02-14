#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// CONFIGURATION
// ==========================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Hardware Pins
const int MOISTURE_PIN = 34; // Analog Input
const int PUMP_PIN = 23;     // Digital Output (Pump or Blue LED)

// Calibration (Adjust these based on your sensor)
const int DRY_VAL = 4095; // Value when sensor is in air
const int WET_VAL = 1500; // Value when sensor is in water

// Pump Timing
const unsigned long PUMP_DURATION_MS = 5000; // 5 seconds

// ==========================================
// GLOBALS
// ==========================================
WebServer server(80);
bool isPumpOn = false;
unsigned long pumpStartTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Hardware Init
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // Off by default
  pinMode(MOISTURE_PIN, INPUT);

  // WiFi Init
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // API Routes
  server.on("/", handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/water", HTTP_POST, handleWater);
  server.on("/water", HTTP_OPTIONS, handleOptions); // CORS Preflight
  server.on("/status", HTTP_OPTIONS, handleOptions); // CORS Preflight

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  handlePumpLogic();
}

// ==========================================
// HANDLERS
// ==========================================

// Enable CORS for all responses to allow the local HTML file to talk to ESP32
void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  sendCorsHeaders();
  server.send(204); // No Content
}

void handleRoot() {
  sendCorsHeaders();
  server.send(200, "text/plain", "Gravity Meter API is Running. Use /status or /water.");
}

void handleStatus() {
  sendCorsHeaders();
  
  // Read Sensor
  int rawVal = analogRead(MOISTURE_PIN);
  
  // Map to percentage (0 = dry, 100 = wet)
  // Note: Capacitive sensors usually have High Value = Dry, Low Value = Wet
  int percent = map(rawVal, DRY_VAL, WET_VAL, 0, 100);
  percent = constrain(percent, 0, 100);

  // Create JSON
  String json = "{";
  json += "\"moisture\": " + String(percent) + ",";
  json += "\"pump\": " + String(isPumpOn ? "true" : "false") + ",";
  json += "\"raw\": " + String(rawVal);
  json += "}";

  server.send(200, "application/json", json);
}

void handleWater() {
  sendCorsHeaders();
  
  if (!isPumpOn) {
    digitalWrite(PUMP_PIN, HIGH);
    isPumpOn = true;
    pumpStartTime = millis();
    Serial.println("Pump turned ON");
  }
  
  server.send(200, "application/json", "{\"status\":\"watering\"}");
}

// Non-blocking pump timer
void handlePumpLogic() {
  if (isPumpOn) {
    if (millis() - pumpStartTime >= PUMP_DURATION_MS) {
      digitalWrite(PUMP_PIN, LOW);
      isPumpOn = false;
      Serial.println("Pump turned OFF");
    }
  }
}
