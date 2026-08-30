#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

// Global App State
int activeCategory = 1; // 1: Names, 2: Screensaver, 3: Game
int frameDelay = 30;
unsigned long lastFrameTime = 0;

// Task 1: Animated Names State
String displayName = "Parvathi";
int nameAnimMode = 1;
int nameTextSize = 2;
float nameX = 128;
float nameY = 24;
float nameSpeedX = 2.0;
float nameSpeedY = 1.5;
float sineAngle = 0.0;

// Task 2: Screensavers State
int saverMode = 1;

const int NUM_STARS = 32;
float starX[NUM_STARS];
float starY[NUM_STARS];
float starZ[NUM_STARS];

float dvdX = 10;
float dvdY = 10;
float dvdSpeedX = 2.0;
float dvdSpeedY = 1.5;
const int dvdW = 32;
const int dvdH = 14;

const int GRID_W = 32;
const int GRID_H = 16;
uint8_t lifeGrid[GRID_W][GRID_H];
uint8_t nextLifeGrid[GRID_W][GRID_H];

const int MATRIX_COLS = 16;
int dropY[MATRIX_COLS];

// Task 3: 2D Game State
const int groundY = 56;
float playerY = groundY - 12;
float playerVelocityY = 0;
const float gravity = 1.2;
const float jumpStrength = -9.0;
bool isGrounded = true;

int obstacleX = 128;
int obstacleWidth = 8;
int obstacleHeight = 14;
int gameSpeed = 4;
int gameScore = 0;
bool gameOver = false;

// HTML Web Interface
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 OLED Master Studio</title>
  <style>
    body { font-family: sans-serif; background: #0b0f19; color: #f1f5f9; text-align: center; margin: 0; padding: 16px; }
    .card { background: #1e293b; max-width: 440px; margin: auto; padding: 20px; border-radius: 16px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    h2 { color: #38bdf8; margin: 0 0 16px 0; font-size: 22px; }
    .nav-tabs { display: flex; gap: 6px; margin-bottom: 18px; }
    .nav-tab { flex: 1; padding: 10px 4px; font-size: 13px; font-weight: 700; border: none; border-radius: 8px; background: #334155; color: #94a3b8; cursor: pointer; }
    .nav-tab.active { background: #0284c7; color: #fff; }
    .panel { display: none; }
    .panel.active { display: block; }
    input[type=text] { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #334155; border-radius: 8px; font-size: 16px; background: #0b0f19; color: #fff; box-sizing: border-box; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 10px; }
    .btn { background: #334155; color: white; border: none; padding: 12px; font-size: 14px; font-weight: 600; border-radius: 8px; cursor: pointer; }
    .btn:active { transform: scale(0.97); }
    .btn-action { background: #0284c7; }
    .btn-game { background: #e11d48; padding: 22px; font-size: 20px; border-radius: 12px; margin-top: 10px; width: 100%; font-weight: bold; }
    .slider-box { margin-top: 15px; text-align: left; }
    .slider-box label { font-size: 13px; color: #94a3b8; display: block; margin-bottom: 5px; }
    input[type=range] { width: 100%; accent-color: #38bdf8; }
  </style>
</head>
<body>
  <div class="card">
    <h2>ESP32 OLED Master Studio</h2>
    
    <div class="nav-tabs">
      <button class="nav-tab active" onclick="switchTab(1)">1. Names</button>
      <button class="nav-tab" onclick="switchTab(2)">2. Saver</button>
      <button class="nav-tab" onclick="switchTab(3)">3. Game</button>
    </div>

    <div id="panel-1" class="panel active">
      <input type="text" id="nameInput" placeholder="Enter Name..." value="Parvathi" oninput="api('/setName?val=' + encodeURIComponent(this.value))">
      <div class="grid">
        <button class="btn btn-action" onclick="api('/setNameAnim?val=1')">Marquee</button>
        <button class="btn btn-action" onclick="api('/setNameAnim?val=2')">Box Bounce</button>
        <button class="btn btn-action" onclick="api('/setNameAnim?val=3')">Sine Wave</button>
        <button class="btn btn-action" onclick="api('/toggleSize')">Toggle Size</button>
      </div>
    </div>

    <div id="panel-2" class="panel">
      <div class="grid">
        <button class="btn btn-action" onclick="api('/setSaver?val=1')">3D Starfield</button>
        <button class="btn btn-action" onclick="api('/setSaver?val=2')">DVD Bounce</button>
        <button class="btn btn-action" onclick="api('/setSaver?val=3')">Game of Life</button>
        <button class="btn btn-action" onclick="api('/setSaver?val=4')">Matrix Rain</button>
      </div>
    </div>

    <div id="panel-3" class="panel">
      <button class="btn btn-game" onclick="api('/jump')">JUMP / START</button>
      <button class="btn" style="margin-top: 10px; width: 100%;" onclick="api('/resetGame')">Restart Game</button>
    </div>

    <div class="slider-box">
      <label>Speed (Fast &larr; &rarr; Slow)</label>
      <input type="range" min="10" max="80" value="30" oninput="api('/setSpeed?val=' + this.value)">
    </div>
  </div>

  <script>
    function switchTab(cat) {
      document.querySelectorAll('.nav-tab').forEach((t, i) => t.classList.toggle('active', i === (cat - 1)));
      document.querySelectorAll('.panel').forEach((p, i) => p.classList.toggle('active', i === (cat - 1)));
      api('/setCategory?val=' + cat);
    }
    function api(endpoint) {
      fetch(endpoint);
    }
  </script>
</body>
</html>
)rawliteral";

void initStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    starX[i] = random(-64, 64);
    starY[i] = random(-32, 32);
    starZ[i] = random(1, 128);
  }
}

void initGameOfLife() {
  for (int x = 0; x < GRID_W; x++) {
    for (int y = 0; y < GRID_H; y++) {
      lifeGrid[x][y] = (random(100) < 30) ? 1 : 0;
    }
  }
}

void initMatrixRain() {
  for (int i = 0; i < MATRIX_COLS; i++) {
    dropY[i] = random(-64, 0);
  }
}

void resetDinoGame() {
  playerY = groundY - 12;
  playerVelocityY = 0;
  isGrounded = true;
  obstacleX = 128;
  gameScore = 0;
  gameSpeed = 4;
  gameOver = false;
}

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleSetCategory() {
  if (server.hasArg("val")) {
    activeCategory = server.arg("val").toInt();
    if (activeCategory == 1) { nameX = SCREEN_WIDTH; }
    if (activeCategory == 2) {
      if (saverMode == 1) initStarfield();
      if (saverMode == 3) initGameOfLife();
      if (saverMode == 4) initMatrixRain();
    }
    if (activeCategory == 3) { resetDinoGame(); }
  }
  server.send(200, "text/plain", "OK");
}

void handleSetName() {
  if (server.hasArg("val")) {
    displayName = server.arg("val");
    nameX = SCREEN_WIDTH;
  }
  server.send(200, "text/plain", "OK");
}

void handleSetNameAnim() {
  if (server.hasArg("val")) {
    nameAnimMode = server.arg("val").toInt();
    nameX = SCREEN_WIDTH;
    nameY = 24;
  }
  server.send(200, "text/plain", "OK");
}

void handleToggleSize() {
  nameTextSize = (nameTextSize == 1) ? 2 : 1;
  server.send(200, "text/plain", "OK");
}

void handleSetSaver() {
  if (server.hasArg("val")) {
    saverMode = server.arg("val").toInt();
    if (saverMode == 1) initStarfield();
    if (saverMode == 3) initGameOfLife();
    if (saverMode == 4) initMatrixRain();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetSpeed() {
  if (server.hasArg("val")) {
    frameDelay = server.arg("val").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void handleJump() {
  if (gameOver) {
    resetDinoGame();
  } else if (isGrounded) {
    playerVelocityY = jumpStrength;
    isGrounded = false;
  }
  server.send(200, "text/plain", "OK");
}

void handleResetGame() {
  resetDinoGame();
  server.send(200, "text/plain", "OK");
}

void renderAnimatedNames() {
  display.setTextSize(nameTextSize);
  int charWidth = (nameTextSize == 1) ? 6 : 12;
  int textHeight = (nameTextSize == 1) ? 8 : 16;
  int textWidth = displayName.length() * charWidth;

  if (nameAnimMode == 1) {
    display.setCursor((int)nameX, (SCREEN_HEIGHT - textHeight) / 2);
    display.print(displayName);
    nameX -= 2.0;
    if (nameX < -textWidth) nameX = SCREEN_WIDTH;
  } 
  else if (nameAnimMode == 2) {
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.setCursor((int)nameX, (int)nameY);
    display.print(displayName);

    nameX += nameSpeedX;
    nameY += nameSpeedY;
    if (nameX <= 2 || (nameX + textWidth) >= SCREEN_WIDTH - 2) nameSpeedX = -nameSpeedX;
    if (nameY <= 2 || (nameY + textHeight) >= SCREEN_HEIGHT - 2) nameSpeedY = -nameSpeedY;
  } 
  else if (nameAnimMode == 3) {
    sineAngle += 0.15;
    nameY = (SCREEN_HEIGHT / 2 - textHeight / 2) + (sin(sineAngle) * 16.0);
    display.setCursor((int)nameX, (int)nameY);
    display.print(displayName);
    nameX -= 1.5;
    if (nameX < -textWidth) nameX = SCREEN_WIDTH;
  }
}

void renderScreensavers() {
  switch (saverMode) {
    case 1: {
      int originX = SCREEN_WIDTH / 2;
      int originY = SCREEN_HEIGHT / 2;
      for (int i = 0; i < NUM_STARS; i++) {
        starZ[i] -= 2.0;
        if (starZ[i] <= 0) {
          starX[i] = random(-64, 64);
          starY[i] = random(-32, 32);
          starZ[i] = 128;
        }
        int px = originX + (int)((starX[i] / starZ[i]) * 64.0);
        int py = originY + (int)((starY[i] / starZ[i]) * 64.0);
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
          display.drawPixel(px, py, SSD1306_WHITE);
          if (starZ[i] < 40) {
            display.drawPixel(px + 1, py, SSD1306_WHITE);
            display.drawPixel(px, py + 1, SSD1306_WHITE);
          }
        }
      }
      break;
    }

    case 2: {
      dvdX += dvdSpeedX;
      dvdY += dvdSpeedY;
      if (dvdX <= 0 || dvdX + dvdW >= SCREEN_WIDTH) dvdSpeedX = -dvdSpeedX;
      if (dvdY <= 0 || dvdY + dvdH >= SCREEN_HEIGHT) dvdSpeedY = -dvdSpeedY;

      display.drawRoundRect((int)dvdX, (int)dvdY, dvdW, dvdH, 3, SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor((int)dvdX + 7, (int)dvdY + 3);
      display.print("DVD");
      break;
    }

    case 3: {
      for (int x = 0; x < GRID_W; x++) {
        for (int y = 0; y < GRID_H; y++) {
          if (lifeGrid[x][y]) display.fillRect(x * 4, y * 4, 3, 3, SSD1306_WHITE);
        }
      }
      for (int x = 0; x < GRID_W; x++) {
        for (int y = 0; y < GRID_H; y++) {
          int neighbors = 0;
          for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
              if (dx == 0 && dy == 0) continue;
              int nx = (x + dx + GRID_W) % GRID_W;
              int ny = (y + dy + GRID_H) % GRID_H;
              neighbors += lifeGrid[nx][ny];
            }
          }
          if (lifeGrid[x][y] == 1 && (neighbors == 2 || neighbors == 3)) nextLifeGrid[x][y] = 1;
          else if (lifeGrid[x][y] == 0 && neighbors == 3) nextLifeGrid[x][y] = 1;
          else nextLifeGrid[x][y] = 0;
        }
      }
      for (int x = 0; x < GRID_W; x++) {
        for (int y = 0; y < GRID_H; y++) lifeGrid[x][y] = nextLifeGrid[x][y];
      }
      break;
    }

    case 4: {
      display.setTextSize(1);
      for (int i = 0; i < MATRIX_COLS; i++) {
        int colX = i * 8;
        char randomChar = (char)random(33, 126);
        display.setCursor(colX, dropY[i]);
        display.print(randomChar);
        dropY[i] += 4;
        if (dropY[i] > SCREEN_HEIGHT) dropY[i] = random(-20, 0);
      }
      break;
    }
  }
}

void renderGame() {
  if (gameOver) {
    display.setTextSize(2);
    display.setCursor(10, 12);
    display.print("GAME OVER");
    display.setTextSize(1);
    display.setCursor(20, 36);
    display.print("Final Score: ");
    display.print(gameScore);
    display.setCursor(14, 50);
    display.print("Tap JUMP to Retry");
    return;
  }

  playerY += playerVelocityY;
  playerVelocityY += gravity;
  if (playerY >= groundY - 12) {
    playerY = groundY - 12;
    playerVelocityY = 0;
    isGrounded = true;
  }

  obstacleX -= gameSpeed;
  if (obstacleX < -obstacleWidth) {
    obstacleX = SCREEN_WIDTH;
    gameScore++;
    if (gameScore % 5 == 0 && gameSpeed < 8) gameSpeed++;
  }

  int playerX = 16;
  int playerWidth = 10;
  int playerHeight = 12;
  int obstacleY = groundY - obstacleHeight;

  bool collisionX = (playerX + playerWidth >= obstacleX) && (playerX <= obstacleX + obstacleWidth);
  bool collisionY = (playerY + playerHeight >= obstacleY);
  if (collisionX && collisionY) {
    gameOver = true;
  }

  display.drawFastHLine(0, groundY, SCREEN_WIDTH, SSD1306_WHITE);
  display.fillRect(playerX, (int)playerY, playerWidth, playerHeight, SSD1306_WHITE);
  display.drawPixel(playerX + 7, (int)playerY + 3, SSD1306_BLACK);
  display.fillRect(obstacleX, obstacleY, obstacleWidth, obstacleHeight, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(gameScore);
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(15, 24);
  display.print("Starting...");
  display.display();

  initStarfield();
  initGameOfLife();
  initMatrixRain();
  resetDinoGame();

  WiFi.softAP("ESP32-Control", "12345678");

  server.on("/", handleRoot);
  server.on("/setCategory", handleSetCategory);
  server.on("/setName", handleSetName);
  server.on("/setNameAnim", handleSetNameAnim);
  server.on("/toggleSize", handleToggleSize);
  server.on("/setSaver", handleSetSaver);
  server.on("/setSpeed", handleSetSpeed);
  server.on("/jump", handleJump);
  server.on("/resetGame", handleResetGame);
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastFrameTime >= frameDelay) {
    lastFrameTime = millis();
    display.clearDisplay();

    if (activeCategory == 1) {
      renderAnimatedNames();
    } else if (activeCategory == 2) {
      renderScreensavers();
    } else if (activeCategory == 3) {
      renderGame();
    }

    display.display();
  }
}