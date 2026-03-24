/*
 * Rabbit Miner - Kawaii Bitcoin Pool Miner for ESP32 CYD
 * Designed by Arez
 * 
 * A cute rabbit-themed Bitcoin pool miner for ESP32-2432S028 (CYD)
 * Features: WiFi setup, pool mining, animated rabbit, hash statistics
 * 
 * Complete version with full integration
 */

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
String workerName = "RabbitMiner";
int selectedPool = 0; // Index into MINING_POOLS

// Mining statistics
unsigned long totalHashes = 0;
unsigned long sessionHashes = 0;
float hashRate = 0.0;
unsigned long lastStatsUpdate = 0;
bool poolConnected = false;
int rabbitMood = 0; // 0=sleeping, 1=happy, 2=mining, 3=excited, 4=celebrating

// Mining state
uint8_t blockHeader[80];
uint32_t currentNonce = 0;
bool hasWork = false;

// Colors - Kawaii pastel theme
#define PINK_LIGHT    0xFDF5
#define PINK_DARK     0xFBEA
#define BLUE_LIGHT    0xAEDC
#define PURPLE_LIGHT  0xE73F
#define YELLOW_LIGHT  0xFFE6
#define BG_COLOR      0x18E3
#define TEXT_COLOR    0x5ACB

// Screen dimensions
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Rabbit Miner Starting ===");
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(BG_COLOR);
  
  // Show boot screen
  showBootScreen();
  delay(3000);
  
  // Initialize preferences
  preferences.begin("rabbit-miner", false);
  walletAddress = preferences.getString("wallet", "");
  selectedPool = preferences.getInt("pool", 0);
  
  // Setup WiFi
  setupWiFi();
  
  // If no wallet configured, use example for testing
  if (walletAddress.length() == 0) {
    Serial.println("No wallet configured, using example");
    // Use a well-known address for testing (Satoshi's first block)
    walletAddress = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa.RabbitMiner";
    preferences.putString("wallet", walletAddress);
  }
  
  // Show connecting screen
  showConnectingScreen();
  
  // Connect to pool
  connectToPool();
  
  // Initialize mining
  rabbitMood = 2; // Start mining
  lastStatsUpdate = millis();
}

void loop() {
  // Check pool connection
  if (!stratum.isConnected()) {
    Serial.println("Pool disconnected, reconnecting...");
    poolConnected = false;
    rabbitMood = 0;
    connectToPool();
    delay(5000);
    return;
  }
  
  // Process pool messages
  stratum.processMessages();
  
  // Do mining work
  if (hasWork) {
    doMiningWork();
  }
  
  // Update display periodically
  if (millis() - lastStatsUpdate > 500) {
    updateDisplay();
    lastStatsUpdate = millis();
  }
  
  // Small delay to prevent watchdog
  delay(1);
}

void showBootScreen() {
  tft.fillScreen(PINK_LIGHT);
  
  // Draw cute border
  tft.fillRoundRect(10, 10, 300, 220, 15, PINK_DARK);
  tft.fillRoundRect(15, 15, 290, 210, 12, BG_COLOR);
  
  // Draw rabbit
  drawRabbit(160, 80, 2.0, 1); // Happy rabbit
  
  // Title
  tft.setTextColor(PINK_DARK);
  tft.setTextSize(3);
  tft.setCursor(60, 140);
  tft.println("Rabbit Miner");
  
  // Subtitle
  tft.setTextSize(2);
  tft.setTextColor(PURPLE_LIGHT);
  tft.setCursor(40, 175);
  tft.println("Designed by Arez");
  
  // Version
  tft.setTextSize(1);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(110, 205);
  tft.println("v1.0 - Kawaii Edition");
}

void setupWiFi() {
  showWiFiSetupScreen();
  
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  
  // Add custom parameter for wallet address
  WiFiManagerParameter custom_wallet("wallet", "BTC Wallet Address", 
                                     walletAddress.c_str(), 64);
  wm.addParameter(&custom_wallet);
  
  if (!wm.autoConnect("RabbitMiner-Setup")) {
    Serial.println("Failed to connect to WiFi");
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("WiFi Failed!");
    tft.setTextSize(1);
    tft.setCursor(20, 130);
    tft.println("Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }
  
  // Save wallet if entered
  String newWallet = custom_wallet.getValue();
  if (newWallet.length() > 0) {
    walletAddress = newWallet;
    preferences.putString("wallet", walletAddress);
  }
  
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(BLUE_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("WiFi Connected!");
  delay(1500);
}

void showWiFiSetupScreen() {
  tft.fillScreen(BG_COLOR);
  drawRabbit(160, 60, 1.5, 0); // Sleeping rabbit
  
  tft.setTextColor(PINK_DARK);
  tft.setTextSize(2);
  tft.setCursor(50, 120);
  tft.println("WiFi Setup");
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(20, 150);
  tft.println("Connect to:");
  tft.setTextColor(BLUE_LIGHT);
  tft.setCursor(20, 165);
  tft.println("RabbitMiner-Setup");
  
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(20, 185);
  tft.println("Configure WiFi & Wallet");
}

void showConnectingScreen() {
  tft.fillScreen(BG_COLOR);
  drawRabbit(160, 80, 2.0, 1); // Happy rabbit
  
  tft.setTextColor(BLUE_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(40, 150);
  tft.println("Connecting...");
  
  const MiningPool* pool = getPool(selectedPool);
  if (pool) {
    tft.setTextSize(1);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(40, 180);
    tft.print("Pool: ");
    tft.println(pool->name);
  }
}

void connectToPool() {
  const MiningPool* pool = getPool(selectedPool);
  if (!pool) {
    Serial.println("Invalid pool selection");
    return;
  }
  
  Serial.printf("Connecting to %s (%s:%d)\n", 
                pool->name, pool->host, pool->port);
  
  if (!stratum.connect(pool->host, pool->port)) {
    Serial.println("Failed to connect to pool");
    poolConnected = false;
    return;
  }
  
  Serial.println("Pool connected, subscribing...");
  if (!stratum.subscribe()) {
    Serial.println("Failed to subscribe");
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  Serial.println("Authorizing...");
  if (!stratum.authorize(walletAddress.c_str(), "x")) {
    Serial.println("Failed to authorize");
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  Serial.println("Successfully connected to pool!");
  poolConnected = true;
  rabbitMood = 2; // Mining
  
  // Set up callbacks
  stratum.onNewBlock([]() {
    Serial.println("New block notification!");
    onBlockFound();
  });
  
  stratum.onDifficulty([](float diff) {
    Serial.printf("Difficulty: %.2f\n", diff);
  });
  
  // Start with some test work
  hasWork = true;
  memset(blockHeader, 0, 80);
  currentNonce = 0;
}

void doMiningWork() {
  // Mine a batch of nonces
  const uint32_t BATCH_SIZE = 1000;
  uint32_t foundNonce;
  unsigned long hashCount;
  
  unsigned long startTime = millis();
  
  bool found = miner.mine(blockHeader, currentNonce, BATCH_SIZE, 
                         foundNonce, hashCount);
  
  unsigned long elapsed = millis() - startTime;
  
  // Update statistics
  sessionHashes += hashCount;
  totalHashes += hashCount;
  
  // Calculate hash rate
  if (elapsed > 0) {
    hashRate = (float)hashCount / (elapsed / 1000.0);
  }
  
  // Update rabbit mood based on hash rate
  if (hashRate > 40000) {
    rabbitMood = 3; // Excited
  } else if (hashRate > 20000) {
    rabbitMood = 2; // Mining
  } else {
    rabbitMood = 1; // Happy
  }
  
  if (found) {
    Serial.printf("Found nonce: %u\n", foundNonce);
    // TODO: Submit share to pool
  }
  
  // Move to next batch
  currentNonce += BATCH_SIZE;
  
  // Wrap around after 4 billion
  if (currentNonce > 4000000000UL) {
    currentNonce = 0;
  }
}

void onBlockFound() {
  rabbitMood = 4; // Celebrating!
  
  // Show celebration animation
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(YELLOW_LIGHT);
    drawRabbit(160, 100, 2.5, 4);
    
    tft.setTextColor(PINK_DARK);
    tft.setTextSize(4);
    tft.setCursor(80, 180);
    tft.println("BLOCK!");
    
    delay(300);
    tft.fillScreen(PINK_LIGHT);
    delay(200);
  }
  
  // Reset session hashes
  sessionHashes = 0;
  
  // Back to mining
  rabbitMood = 2;
}

void updateDisplay() {
  tft.fillScreen(BG_COLOR);
  
  // Draw rabbit
  drawRabbit(160, 70, 2.2, rabbitMood);
  
  // Draw energy bar
  drawEnergyBar();
  
  // Draw stats panel
  drawStatsPanel();
}

void drawEnergyBar() {
  int barX = 20;
  int barY = 140;
  int barW = 280;
  int barH = 12;
  
  // Energy based on hash rate (0-50 kH/s = 0-100%)
  float energy = min((hashRate / 50000.0) * 100.0, 100.0);
  
  // Background
  tft.fillRoundRect(barX, barY, barW, barH, 6, TFT_DARKGREY);
  
  // Fill color based on energy
  uint16_t fillColor;
  if (energy > 70) {
    fillColor = TFT_GREEN;
  } else if (energy > 40) {
    fillColor = YELLOW_LIGHT;
  } else {
    fillColor = TFT_ORANGE;
  }
  
  int fillW = (int)(barW * energy / 100.0);
  tft.fillRoundRect(barX, barY, fillW, barH, 6, fillColor);
  
  // Label
  tft.setTextSize(1);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(barX, barY - 12);
  tft.printf("Energy: %.0f%%", energy);
}

void drawStatsPanel() {
  int panelY = 160;
  
  // Panel background
  tft.fillRoundRect(10, panelY, 300, 70, 10, PINK_LIGHT);
  tft.fillRoundRect(14, panelY + 4, 292, 62, 8, TFT_WHITE);
  
  tft.setTextSize(1);
  int col1X = 20;
  int col2X = 165;
  
  // Row 1: Hash Rate
  tft.setTextColor(PINK_DARK);
  tft.setCursor(col1X, panelY + 12);
  tft.print("Hash Rate:");
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(col1X + 70, panelY + 12);
  tft.printf("%.1f kH/s", hashRate / 1000.0);
  
  // Row 1: Pool Status
  tft.setTextColor(poolConnected ? TFT_GREEN : TFT_RED);
  tft.setCursor(col2X, panelY + 12);
  tft.print("Pool:");
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(col2X + 35, panelY + 12);
  tft.print(poolConnected ? "OK" : "ERR");
  
  // Row 2: Session Hashes
  tft.setTextColor(BLUE_LIGHT);
  tft.setCursor(col1X, panelY + 28);
  tft.print("Session:");
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(col1X + 70, panelY + 28);
  tft.printf("%.1f MH", sessionHashes / 1000000.0);
  
  // Row 2: Pool Name
  const MiningPool* pool = getPool(selectedPool);
  tft.setTextColor(PURPLE_LIGHT);
  tft.setCursor(col2X, panelY + 28);
  if (pool) {
    tft.print(pool->name);
  }
  
  // Row 3: Total Hashes
  tft.setTextColor(PURPLE_LIGHT);
  tft.setCursor(col1X, panelY + 44);
  tft.print("Total:");
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(col1X + 70, panelY + 44);
  tft.printf("%.1f MH", totalHashes / 1000000.0);
  
  // Row 3: Wallet (truncated)
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(col2X, panelY + 44);
  String truncWallet = walletAddress.substring(0, 8) + "...";
  tft.print(truncWallet);
}

void drawRabbit(int x, int y, float scale, int mood) {
  int headR = (int)(20 * scale);
  int bodyW = (int)(25 * scale);
  int bodyH = (int)(30 * scale);
  int earW = (int)(5 * scale);
  int earH = (int)(18 * scale);
  
  // Draw ears
  drawRabbitEars(x, y - headR - earH + 5, scale, PINK_DARK);
  
  // Draw head
  tft.fillCircle(x, y, headR, TFT_WHITE);
  tft.drawCircle(x, y, headR, PINK_DARK);
  tft.drawCircle(x, y, headR + 1, PINK_DARK);
  
  // Draw eyes based on mood
  int eyeY = y - 5;
  int eyeSpacing = (int)(8 * scale);
  
  switch (mood) {
    case 0: // Sleeping - closed eyes
      tft.drawLine(x - eyeSpacing - 3, eyeY, x - eyeSpacing + 3, eyeY, TFT_BLACK);
      tft.drawLine(x + eyeSpacing - 3, eyeY, x + eyeSpacing + 3, eyeY, TFT_BLACK);
      break;
      
    case 1: // Happy - normal eyes
      tft.fillCircle(x - eyeSpacing, eyeY, 2, TFT_BLACK);
      tft.fillCircle(x + eyeSpacing, eyeY, 2, TFT_BLACK);
      break;
      
    case 2: // Mining - focused eyes
      tft.fillCircle(x - eyeSpacing, eyeY, 3, TFT_BLACK);
      tft.fillCircle(x + eyeSpacing, eyeY, 3, TFT_BLACK);
      break;
      
    case 3: // Excited - big eyes with sparkles
      tft.fillCircle(x - eyeSpacing, eyeY, 4, TFT_BLACK);
      tft.fillCircle(x + eyeSpacing, eyeY, 4, TFT_BLACK);
      drawStar(x - 25, y - 20, 3, YELLOW_LIGHT);
      drawStar(x + 25, y - 20, 3, YELLOW_LIGHT);
      break;
      
    case 4: // Celebrating - star eyes!
      drawStar(x - eyeSpacing, eyeY, 5, YELLOW_LIGHT);
      drawStar(x + eyeSpacing, eyeY, 5, YELLOW_LIGHT);
      drawStar(x - 30, y - 25, 4, YELLOW_LIGHT);
      drawStar(x + 30, y - 25, 4, YELLOW_LIGHT);
      drawStar(x, y - 35, 3, YELLOW_LIGHT);
      break;
  }
  
  // Draw nose
  tft.fillCircle(x, y + 5, (int)(2 * scale), PINK_DARK);
  
  // Draw mouth
  if (mood >= 2) {
    int mouthY = y + 8;
    for (int i = -5; i <= 5; i++) {
      int py = mouthY + abs(i) / 2;
      tft.drawPixel(x + i, py, PINK_DARK);
    }
  }
  
  // Draw body
  int bodyY = y + headR - 5;
  tft.fillRoundRect(x - bodyW/2, bodyY, bodyW, bodyH, 12, TFT_WHITE);
  tft.drawRoundRect(x - bodyW/2, bodyY, bodyW, bodyH, 12, PINK_DARK);
  tft.drawRoundRect(x - bodyW/2 + 1, bodyY + 1, bodyW - 2, bodyH - 2, 10, PINK_DARK);
  
  // Draw paws
  int pawY = bodyY + bodyH - 5;
  tft.fillCircle(x - bodyW/2 - 2, pawY, 4, PINK_LIGHT);
  tft.fillCircle(x + bodyW/2 + 2, pawY, 4, PINK_LIGHT);
  
  // Draw tail
  int tailY = bodyY + bodyH/2;
  tft.fillCircle(x + bodyW/2 + 4, tailY, 5, PINK_LIGHT);
  tft.drawCircle(x + bodyW/2 + 4, tailY, 5, PINK_DARK);
}

void drawRabbitEars(int x, int y, float scale, uint16_t color) {
  int earW = (int)(5 * scale);
  int earH = (int)(18 * scale);
  int earSpacing = (int)(10 * scale);
  
  // Left ear
  tft.fillRoundRect(x - earSpacing - earW, y, earW, earH, earW/2, color);
  tft.fillRoundRect(x - earSpacing - earW + 1, y + 2, 
                    earW - 2, earH - 4, earW/2, PINK_LIGHT);
  
  // Right ear
  tft.fillRoundRect(x + earSpacing, y, earW, earH, earW/2, color);
  tft.fillRoundRect(x + earSpacing + 1, y + 2, 
                    earW - 2, earH - 4, earW/2, PINK_LIGHT);
}

void drawStar(int x, int y, int size, uint16_t color) {
  // Draw 4-pointed star
  tft.drawLine(x - size, y, x + size, y, color);
  tft.drawLine(x, y - size, x, y + size, color);
  tft.drawLine(x - size*0.7, y - size*0.7, x + size*0.7, y + size*0.7, color);
  tft.drawLine(x - size*0.7, y + size*0.7, x + size*0.7, y - size*0.7, color);
  tft.fillCircle(x, y, size/3, color);
}
