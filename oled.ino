#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize SSD1306 OLED display connected via I2C (address 0x3C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create Web Server on Port 80
WebServer server(80);

// HTML Page served to phone/laptop browser
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f4f4f9; }
    h2 { color: #333; }
    .btn { background-color: #4CAF50; color: white; padding: 15px 32px; font-size: 18px; border: none; border-radius: 8px; cursor: pointer; margin: 10px; }
    .btn-clear { background-color: #f44336; }
  </style>
</head>
<body>
  <h2>ESP32 OLED Control</h2>
  <button class="btn" onclick="sendReq('/hello')">Say Hello</button>
  <button class="btn" onclick="sendReq('/status')">Show Status</button>
  <button class="btn btn-clear" onclick="sendReq('/clear')">Clear Screen</button>

  <script>
    function sendReq(path) {
      fetch(path);
    }
  </script>
</body>
</html>
)rawliteral";

void updateDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(message);
  display.display();
}

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleHello() {
  updateDisplay(" Hello\n buddy!");
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  updateDisplay(" Status:\n ONLINE");
  server.send(200, "text/plain", "OK");
}

void handleClear() {
  display.clearDisplay();
  display.display();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // 1. Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  updateDisplay("  Booting...");

  // 2. Start Wi-Fi Access Point (SoftAP)
  WiFi.softAP("ESP32-Control", "12345678");
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // 3. Define HTTP Route Handlers
  server.on("/", handleRoot);
  server.on("/hello", handleHello);
  server.on("/status", handleStatus);
  server.on("/clear", handleClear);

  server.begin();
  
  delay(1000);
  updateDisplay(" Ready!\n Connect WiFi");
}

void loop() {
  // Listen for incoming HTTP client requests from phone/laptop
  server.handleClient();
}