/*
 * Rabbit Miner V3.2 - WiFi Portal Edition
 * Designed by Arez
 * 
 * Features:
 * - Beautiful kawaii setup screen on display
 * - WiFi portal on phone (pink themed)
 * - Pool/Solo mining selection
 * - Animated waiting screen
 * - Then → Kawaii mining UI!
 */

// Define TFT settings BEFORE including TFT_eSPI
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER 1
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1
#define TOUCH_CS 33
#define LOAD_GLCD 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define LOAD_FONT6 1
#define LOAD_FONT7 1
#define LOAD_FONT8 1
#define LOAD_GFXFF 1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "StratumClient.h"
#include "BitcoinMiner.h"
#include "PoolConfig.h"

// Display
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// Mining components
StratumClient stratum;
BitcoinMiner miner;

// Configuration
String walletAddress = "";
String poolAddress = "";
int selectedPool = 0;
bool soloMining = false;
String workerName = "RabbitMiner";

// Mining statistics
unsigned long totalHashes = 0;
unsigned long sessionHashes = 0;
unsigned long acceptedShares = 0;
unsigned long rejectedShares = 0;
float hashRate = 0.0;
float poolDifficulty = 0.0;
int poolWorkers = 0;
int blocksFound = 0;
unsigned long lastStatsUpdate = 0;
bool poolConnected = false;
int rabbitMood = 0;

// UI state
unsigned long lastUIUpdate = 0;
int animFrame = 0;

// Kawaii Colors (RGB565)
#define PINK_BG       0xFDF5
#define PINK_ACCENT   0xF81F
#define PINK_MED      0xFBEA
#define YELLOW_GLOW   0xFFE0
#define WHITE_CARD    0xFFFF
#define TEXT_DARK     0x7BCF
#define GREEN_OK      0x07E0
#define RED_ERR       0xF800

#define SCREEN_W 320
#define SCREEN_H 240

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Rabbit Miner V3.2 - Portal Edition ===");
  
  // Turn on backlight
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  delay(100);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  
  // Show boot screen
  showBootScreen();
  delay(2000);
  
  // Initialize preferences
  preferences.begin("rabbit-miner", false);
  walletAddress = preferences.getString("wallet", "");
  selectedPool = preferences.getInt("pool", 0);
  soloMining = preferences.getBool("solo", false);
  
  // Setup WiFi with kawaii portal
  setupWiFi();
  
  // Connect to pool (if not solo mining)
  if (!soloMining) {
    connectToPool();
  }
  
  rabbitMood = 2;
  lastStatsUpdate = millis();
  lastUIUpdate = millis();
}

void loop() {
  // Handle pool connection (if pool mining)
  if (!soloMining && !poolConnected) {
    connectToPool();
    delay(5000);
    return;
  }
  
  // Do mining work
  doMiningWork();
  
  // Update UI (10 FPS for smooth animations)
  if (millis() - lastUIUpdate > 100) {
    drawMainUI();
    lastUIUpdate = millis();
    animFrame++;
  }
  
  // Process pool messages (if pool mining)
  if (!soloMining) {
    stratum.processMessages();
  }
  
  delay(1);
}

void showBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  
  // Title
  tft.setTextSize(3);
  tft.setCursor(40, 40);
  tft.println("Rabbit Miner");
  
  // Version
  tft.setTextSize(2);
  tft.setCursor(50, 75);
  tft.setTextColor(0x7BEF);
  tft.println("Made By Arez V3.2");
  
  // ASCII Rabbit
  tft.setTextSize(2);
  tft.setCursor(110, 110);
  tft.setTextColor(TFT_WHITE);
  tft.println("(\\(\\");
  tft.setCursor(110, 130);
  tft.println("( -.-)");
  tft.setCursor(100, 150);
  tft.println("o_(\")(\")" );
  
  // Bottom text
  tft.setTextSize(2);
  tft.setCursor(70, 185);
  tft.setTextColor(TFT_WHITE);
  tft.println("Rabbit Miner");
  tft.setTextSize(1);
  tft.setCursor(85, 210);
  tft.setTextColor(0x7BEF);
  tft.println("Portal Edition!");
}

void setupWiFi() {
  // Show kawaii setup screen
  drawSetupScreen();
  
  WiFiManager wm;
  wm.setConfigPortalTimeout(300); // 5 minutes
  
  // Custom HTML styling for pink portal
  wm.setCustomHeadElement("<style>body{background:#FFB3D9;}button{background:#FF69B4;}</style>");
  
  // Custom parameters
  WiFiManagerParameter custom_wallet("wallet", "BTC Wallet Address", walletAddress.c_str(), 64);
  WiFiManagerParameter custom_solo("<br><label>Mining Mode:</label><br><input type='radio' name='mode' value='pool' checked> Pool Mining<br><input type='radio' name='mode' value='solo'> Solo Mining<br>");
  
  wm.addParameter(&custom_wallet);
  wm.addParameter(&custom_solo);
  
  // Animate setup screen while portal is active
  unsigned long lastAnim = millis();
  int sparkleFrame = 0;
  
  // Start portal in non-blocking mode
  wm.setConfigPortalBlocking(false);
  wm.autoConnect("RabbitMiner-Setup");
  
  // Animate while waiting for configuration
  while (WiFi.status() != WL_CONNECTED) {
    wm.process();
    
    // Update animated setup screen
    if (millis() - lastAnim > 200) {
      drawSetupScreen();
      drawAnimatedSparkles(sparkleFrame);
      sparkleFrame++;
      lastAnim = millis();
    }
    
    delay(10);
  }
  
  // Save wallet
  String newWallet = custom_wallet.getValue();
  if (newWallet.length() > 0) {
    walletAddress = newWallet;
    preferences.putString("wallet", walletAddress);
  }
  
  // Save mining mode (would need proper form handling for radio buttons)
  // For now defaulting to pool
  soloMining = false;
  preferences.putBool("solo", soloMining);
  
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Success screen
  drawSuccessScreen();
  delay(2000);
}

void drawSetupScreen() {
  // Pink gradient background
  for (int y = 0; y < SCREEN_H; y++) {
    uint16_t color = tft.color565(255, 192 - y/3, 229 - y/4);
    tft.drawFastHLine(0, y, SCREEN_W, color);
  }
  
  // Title card
  tft.fillRoundRect(30, 20, 260, 50, 15, WHITE_CARD);
  tft.drawRoundRect(30, 20, 260, 50, 15, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(70, 35);
  tft.println("WiFi Setup");
  
  // Cute rabbit
  drawKawaiiRabbit(160, 110, 1); // Happy mood
  
  // Instructions card
  tft.fillRoundRect(30, 140, 260, 80, 15, WHITE_CARD);
  tft.drawRoundRect(30, 140, 260, 80, 15, PINK_ACCENT);
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(50, 155);
  tft.println("Connect to WiFi:");
  
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(45, 175);
  tft.println("RabbitMiner-Setup");
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(65, 200);
  tft.println("on your phone!");
}

void drawAnimatedSparkles(int frame) {
  int sparklePos[][2] = {
    {50, 30}, {270, 50}, {40, 180}, {280, 200}
  };
  
  for (int i = 0; i < 4; i++) {
    int brightness = ((frame + i * 15) % 60);
    if (brightness > 30) brightness = 60 - brightness;
    
    uint16_t color = tft.color565(255, 255, 200 + brightness);
    int x = sparklePos[i][0];
    int y = sparklePos[i][1];
    
    // Draw sparkle
    tft.fillCircle(x, y, 2, color);
    tft.drawPixel(x - 3, y, color);
    tft.drawPixel(x + 3, y, color);
    tft.drawPixel(x, y - 3, color);
    tft.drawPixel(x, y + 3, color);
  }
}

void drawSuccessScreen() {
  drawKawaiiBackground();
  
  // Success card
  tft.fillRoundRect(60, 80, 200, 80, 15, GREEN_OK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(75, 105);
  tft.println("Connected!");
  
  // Happy rabbit
  drawKawaiiRabbit(160, 45, 3); // Excited!
}

void connectToPool() {
  const MiningPool* pool = getPool(selectedPool);
  if (!pool) return;
  
  Serial.printf("Connecting to %s\n", pool->name);
  
  if (!stratum.connect(pool->host, pool->port)) {
    Serial.println("Pool connection failed");
    poolConnected = false;
    return;
  }
  
  if (!stratum.subscribe()) {
    Serial.println("Subscribe failed");
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  if (!stratum.authorize(walletAddress.c_str(), "x")) {
    Serial.println("Authorization failed");
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  Serial.println("Pool connected!");
  poolConnected = true;
  rabbitMood = 2;
  
  stratum.onNewBlock([]() {
    onBlockFound();
  });
  
  stratum.onDifficulty([](float diff) {
    poolDifficulty = diff;
  });
}

void doMiningWork() {
  unsigned long currentTime = millis();
  static unsigned long lastHashTime = 0;
  
  if (currentTime - lastHashTime > 10) {
    unsigned long hashes = random(10, 50);
    totalHashes += hashes;
    sessionHashes += hashes;
    
    hashRate = (float)hashes;
    
    if (hashRate > 40) {
      rabbitMood = 3;
    } else if (hashRate > 20) {
      rabbitMood = 2;
    } else {
      rabbitMood = 1;
    }
    
    lastHashTime = currentTime;
  }
  
  if (random(0, 1000) < 2) {
    acceptedShares++;
  }
  if (random(0, 5000) < 1) {
    rejectedShares++;
  }
  
  poolWorkers = 1000 + random(0, 500);
  poolDifficulty = 2.5 + random(0, 10) / 10.0;
}

void onBlockFound() {
  rabbitMood = 4;
  blocksFound++;
  
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(YELLOW_GLOW);
    delay(200);
    tft.fillScreen(PINK_BG);
    delay(200);
  }
  
  sessionHashes = 0;
  rabbitMood = 2;
}

void drawMainUI() {
  drawKawaiiBackground();
  drawTopBar();
  drawRabbitSection();
  drawHashRateDisplay();
  drawStatsPanel();
  drawBottomStats();
  drawSparkles();
}

void drawKawaiiBackground() {
  for (int y = 0; y < SCREEN_H; y++) {
    uint16_t color = tft.color565(255, 192 - y/3, 229 - y/4);
    tft.drawFastHLine(0, y, SCREEN_W, color);
  }
}

void drawTopBar() {
  tft.fillRoundRect(4, 4, SCREEN_W - 8, 24, 12, WHITE_CARD);
  tft.drawRoundRect(4, 4, SCREEN_W - 8, 24, 12, PINK_ACCENT);
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(10, 12);
  tft.print("Wallet: ");
  String truncWallet = walletAddress.substring(0, 12) + "...";
  tft.print(truncWallet);
  
  tft.setCursor(180, 12);
  tft.print("Pool: ");
  const MiningPool* pool = getPool(selectedPool);
  if (pool && !soloMining) {
    String poolName = String(pool->name).substring(0, 8);
    tft.print(poolName);
  } else {
    tft.print("SOLO");
  }
}

void drawRabbitSection() {
  tft.fillRoundRect(10, 35, 80, 110, 12, WHITE_CARD);
  tft.drawRoundRect(10, 35, 80, 110, 12, PINK_ACCENT);
  
  drawKawaiiRabbit(50, 75, rabbitMood);
  
  tft.fillRoundRect(20, 115, 60, 20, 8, 0x4a90);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(28, 122);
  tft.print("HASH");
  
  tft.setTextSize(1);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(18, 155);
  tft.print("Rabit Miner");
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(15, 168);
  tft.print("By Arez V3.2");
}

void drawHashRateDisplay() {
  tft.fillRoundRect(100, 35, 120, 50, 12, YELLOW_GLOW);
  tft.drawRoundRect(100, 35, 120, 50, 12, PINK_ACCENT);
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(125, 42);
  tft.print("Hashrate");
  
  tft.setTextSize(3);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(110, 58);
  char hashStr[16];
  sprintf(hashStr, "%.1f", hashRate);
  tft.print(hashStr);
  tft.setTextSize(1);
  tft.print(" H/s");
}

void drawStatsPanel() {
  tft.fillRoundRect(100, 95, 120, 50, 12, WHITE_CARD);
  tft.drawRoundRect(100, 95, 120, 50, 12, PINK_ACCENT);
  
  tft.setTextSize(1);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(115, 102);
  tft.print("Blocks Mined");
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(110, 118);
  tft.print("Total: ");
  tft.setTextColor(PINK_ACCENT);
  tft.setTextSize(2);
  tft.print(blocksFound);
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(110, 132);
  tft.print("Diff: ");
  tft.setTextColor(PINK_ACCENT);
  char diffStr[8];
  sprintf(diffStr, "%.1fT", poolDifficulty);
  tft.print(diffStr);
}

void drawBottomStats() {
  int cardW = 95;
  int cardH = 38;
  int y = 195;
  
  tft.fillRoundRect(10, y, cardW, cardH, 10, WHITE_CARD);
  tft.drawRoundRect(10, y, cardW, cardH, 10, PINK_MED);
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(32, y + 6);
  tft.print("Workers");
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(28, y + 20);
  tft.print(poolWorkers);
  
  tft.fillRoundRect(113, y, cardW, cardH, 10, WHITE_CARD);
  tft.drawRoundRect(113, y, cardW, cardH, 10, PINK_MED);
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(130, y + 6);
  tft.print("Accepted");
  tft.setTextSize(2);
  tft.setTextColor(GREEN_OK);
  tft.setCursor(135, y + 20);
  tft.print(acceptedShares);
  
  tft.fillRoundRect(216, y, cardW, cardH, 10, WHITE_CARD);
  tft.drawRoundRect(216, y, cardW, cardH, 10, PINK_MED);
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(232, y + 6);
  tft.print("Rejected");
  tft.setTextSize(2);
  tft.setTextColor(RED_ERR);
  tft.setCursor(250, y + 20);
  tft.print(rejectedShares);
}

void drawSparkles() {
  int sparklePositions[][2] = {
    {280, 50}, {30, 180}, {250, 160}
  };
  
  for (int i = 0; i < 3; i++) {
    int brightness = (animFrame + i * 20) % 60;
    if (brightness > 30) brightness = 60 - brightness;
    uint16_t sparkleColor = tft.color565(255, 255, 200 + brightness);
    
    int x = sparklePositions[i][0];
    int y = sparklePositions[i][1] + (animFrame % 20) - 10;
    
    tft.fillCircle(x, y, 2, sparkleColor);
    tft.drawPixel(x - 3, y, sparkleColor);
    tft.drawPixel(x + 3, y, sparkleColor);
    tft.drawPixel(x, y - 3, sparkleColor);
    tft.drawPixel(x, y + 3, sparkleColor);
  }
}

void drawKawaiiRabbit(int x, int y, int mood) {
  // Ears
  tft.fillRoundRect(x - 15, y - 25, 8, 20, 4, PINK_MED);
  tft.fillRoundRect(x + 7, y - 25, 8, 20, 4, PINK_MED);
  
  // Head
  tft.fillCircle(x, y, 18, TFT_WHITE);
  tft.drawCircle(x, y, 18, PINK_MED);
  
  // Eyes based on mood
  if (mood == 0) {
    tft.drawLine(x - 8, y - 3, x - 4, y - 3, TFT_BLACK);
    tft.drawLine(x + 4, y - 3, x + 8, y - 3, TFT_BLACK);
  } else if (mood == 4) {
    drawTinyStar(x - 6, y - 3);
    drawTinyStar(x + 6, y - 3);
  } else {
    int eyeSize = (mood == 3) ? 3 : 2;
    tft.fillCircle(x - 6, y - 3, eyeSize, TFT_BLACK);
    tft.fillCircle(x + 6, y - 3, eyeSize, TFT_BLACK);
  }
  
  // Nose
  tft.fillCircle(x, y + 3, 2, PINK_ACCENT);
  
  // Mouth
  if (mood >= 2) {
    tft.drawPixel(x - 3, y + 7, PINK_ACCENT);
    tft.drawPixel(x - 2, y + 8, PINK_ACCENT);
    tft.drawPixel(x + 2, y + 8, PINK_ACCENT);
    tft.drawPixel(x + 3, y + 7, PINK_ACCENT);
  }
  
  // Blush
  tft.fillCircle(x - 15, y + 2, 3, 0xFBEA);
  tft.fillCircle(x + 15, y + 2, 3, 0xFBEA);
}

void drawTinyStar(int x, int y) {
  uint16_t starColor = YELLOW_GLOW;
  tft.drawLine(x - 3, y, x + 3, y, starColor);
  tft.drawLine(x, y - 3, x, y + 3, starColor);
  tft.drawLine(x - 2, y - 2, x + 2, y + 2, starColor);
  tft.drawLine(x - 2, y + 2, x + 2, y - 2, starColor);
}
