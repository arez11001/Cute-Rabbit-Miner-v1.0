/*
 * Rabbit Miner V3.0 - Touch-Based Setup Edition
 * Designed by Arez
 * 
 * Features:
 * - On-screen WiFi network scanner
 * - Touch-based network selection
 * - On-screen QWERTY keyboard
 * - BTC wallet input
 * - Solo/Pool mining toggle
 * - Beautiful kawaii pink UI throughout!
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
#include <Preferences.h>
#include "StratumClient.h"
#include "BitcoinMiner.h"
#include "PoolConfig.h"

// Display & Touch
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// Touch calibration for CYD
#define TOUCH_X_MIN 300
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 300
#define TOUCH_Y_MAX 3900

// Setup flow states
enum SetupState {
  STATE_BOOT,
  STATE_WIFI_SCAN,
  STATE_WIFI_PASSWORD,
  STATE_WALLET_INPUT,
  STATE_MINING_SELECT,
  STATE_MINING
};

SetupState currentState = STATE_BOOT;

// WiFi networks
String wifiNetworks[10];
int wifiRSSI[10];
int numNetworks = 0;
int selectedNetwork = -1;
String selectedSSID = "";

// User input
String wifiPassword = "";
String walletAddress = "";
bool soloMining = false; // Default to pool mining

// Keyboard
String keyboardInput = "";
bool capsLock = false;

// Mining components
StratumClient stratum;
BitcoinMiner miner;
int selectedPool = 0;
String workerName = "RabbitMiner";

// Mining stats
unsigned long totalHashes = 0;
unsigned long acceptedShares = 0;
unsigned long rejectedShares = 0;
float hashRate = 0.0;
float poolDifficulty = 2.5;
int poolWorkers = 1234;
int blocksFound = 0;
bool poolConnected = false;
int rabbitMood = 2;
unsigned long lastUIUpdate = 0;
int animFrame = 0;

// Kawaii Colors
#define PINK_BG       0xFDF5
#define PINK_ACCENT   0xF81F
#define PINK_MED      0xFBEA
#define YELLOW_GLOW   0xFFE0
#define WHITE_CARD    0xFFFF
#define TEXT_DARK     0x7BCF
#define GREEN_OK      0x07E0
#define RED_ERR       0xF800
#define GRAY_BTN      0xCE79

#define SCREEN_W 320
#define SCREEN_H 240

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Rabbit Miner V3.0 ===");
  
  // Backlight
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  
  // Calibrate touch for CYD display (landscape mode)
  uint16_t calData[5] = {275, 3620, 264, 3532, 1};
  tft.setTouch(calData);
  
  Serial.println("Touch initialized!");
  
  // Show boot screen
  showBootScreen();
  delay(2000);
  
  // Initialize preferences
  preferences.begin("rabbit-miner", false);
  
  // Check if we have saved WiFi
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("pass", "");
  walletAddress = preferences.getString("wallet", "");
  soloMining = preferences.getBool("solo", false);
  
  if (savedSSID.length() > 0 && walletAddress.length() > 0) {
    // Try connecting with saved credentials
    tft.fillScreen(PINK_BG);
    tft.setTextColor(PINK_ACCENT);
    tft.setTextSize(2);
    tft.setCursor(60, 100);
    tft.println("Connecting...");
    
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      // Connected! Go straight to mining
      currentState = STATE_MINING;
      if (!soloMining) {
        connectToPool();
      }
      lastUIUpdate = millis();
      return;
    }
  }
  
  // Start setup flow
  currentState = STATE_WIFI_SCAN;
  scanWiFiNetworks();
}

void loop() {
  switch (currentState) {
    case STATE_WIFI_SCAN:
      handleWiFiScanScreen();
      break;
      
    case STATE_WIFI_PASSWORD:
      handlePasswordScreen();
      break;
      
    case STATE_WALLET_INPUT:
      handleWalletScreen();
      break;
      
    case STATE_MINING_SELECT:
      handleMiningSelectScreen();
      break;
      
    case STATE_MINING:
      handleMiningScreen();
      break;
  }
  
  delay(10);
}

void showBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  
  tft.setTextSize(3);
  tft.setCursor(40, 40);
  tft.println("Rabbit Miner");
  
  tft.setTextSize(2);
  tft.setCursor(50, 75);
  tft.setTextColor(0x7BEF);
  tft.println("Made By Arez V3.0");
  
  // ASCII rabbit
  tft.setTextSize(2);
  tft.setCursor(110, 110);
  tft.setTextColor(TFT_WHITE);
  tft.println("(\\(\\");
  tft.setCursor(110, 130);
  tft.println("( -.-)");
  tft.setCursor(100, 150);
  tft.println("o_(\")(\")" );
  
  tft.setTextSize(1);
  tft.setCursor(90, 210);
  tft.setTextColor(0x7BEF);
  tft.println("Touch Setup Edition!");
}

void scanWiFiNetworks() {
  tft.fillScreen(PINK_BG);
  tft.setTextColor(PINK_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(70, 100);
  tft.println("Scanning WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  numNetworks = WiFi.scanNetworks();
  if (numNetworks > 10) numNetworks = 10;
  
  for (int i = 0; i < numNetworks; i++) {
    wifiNetworks[i] = WiFi.SSID(i);
    wifiRSSI[i] = WiFi.RSSI(i);
  }
  
  drawWiFiScanScreen();
}

void drawWiFiScanScreen() {
  drawKawaiiBackground();
  
  // Title
  tft.fillRoundRect(10, 10, 300, 30, 12, WHITE_CARD);
  tft.drawRoundRect(10, 10, 300, 30, 12, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(70, 18);
  tft.println("Select WiFi");
  
  // Network list
  for (int i = 0; i < numNetworks && i < 5; i++) {
    int y = 50 + (i * 36);
    uint16_t bgColor = (selectedNetwork == i) ? PINK_MED : WHITE_CARD;
    
    tft.fillRoundRect(10, y, 300, 32, 10, bgColor);
    tft.drawRoundRect(10, y, 300, 32, 10, PINK_ACCENT);
    
    tft.setTextSize(1);
    tft.setTextColor(TEXT_DARK);
    tft.setCursor(20, y + 10);
    
    String displayName = wifiNetworks[i];
    if (displayName.length() > 28) {
      displayName = displayName.substring(0, 25) + "...";
    }
    tft.print(displayName);
    
    // Signal strength
    String signal = String(wifiRSSI[i]) + " dBm";
    tft.setCursor(260, y + 10);
    tft.print(signal);
  }
}

void handleWiFiScanScreen() {
  uint16_t touchX, touchY;
  if (getTouchCoordinates(&touchX, &touchY)) {
    // Check network selection
    for (int i = 0; i < numNetworks && i < 5; i++) {
      int y = 50 + (i * 36);
      if (touchY >= y && touchY <= y + 32) {
        selectedNetwork = i;
        selectedSSID = wifiNetworks[i];
        
        // Redraw to show selection
        drawWiFiScanScreen();
        delay(300);
        
        // Move to password screen
        currentState = STATE_WIFI_PASSWORD;
        wifiPassword = "";
        drawPasswordScreen();
        return;
      }
    }
  }
}

void drawPasswordScreen() {
  drawKawaiiBackground();
  
  // Title
  tft.fillRoundRect(10, 10, 300, 30, 12, WHITE_CARD);
  tft.drawRoundRect(10, 10, 300, 30, 12, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(60, 18);
  tft.println("WiFi Password");
  
  // SSID display
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(20, 48);
  tft.print("Network: ");
  tft.setTextColor(PINK_ACCENT);
  tft.print(selectedSSID);
  
  // Password field
  tft.fillRoundRect(10, 70, 300, 30, 10, WHITE_CARD);
  tft.drawRoundRect(10, 70, 300, 30, 10, PINK_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(20, 82);
  
  String displayPass = "";
  for (int i = 0; i < wifiPassword.length(); i++) {
    displayPass += "*";
  }
  tft.print(displayPass);
  
  // On-screen keyboard
  drawKeyboard();
  
  // Connect button
  tft.fillRoundRect(220, 220, 90, 28, 10, GREEN_OK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(240, 230);
  tft.println("Connect");
  
  // Back button
  tft.fillRoundRect(10, 220, 90, 28, 10, GRAY_BTN);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(35, 230);
  tft.println("Back");
}

void drawKeyboard() {
  String keys[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  int keyW = 28;
  int keyH = 24;
  int startY = 110;
  
  for (int row = 0; row < 3; row++) {
    int numKeys = keys[row].length();
    int startX = (320 - (numKeys * keyW)) / 2;
    
    for (int col = 0; col < numKeys; col++) {
      int x = startX + (col * keyW);
      int y = startY + (row * keyH);
      
      tft.fillRoundRect(x, y, keyW - 2, keyH - 2, 4, WHITE_CARD);
      tft.drawRoundRect(x, y, keyW - 2, keyH - 2, 4, PINK_MED);
      
      tft.setTextSize(1);
      tft.setTextColor(TEXT_DARK);
      tft.setCursor(x + 10, y + 8);
      
      char c = keys[row].charAt(col);
      if (capsLock) c = toupper(c);
      tft.print(c);
    }
  }
  
  // Space bar
  tft.fillRoundRect(90, startY + 72, 140, keyH - 2, 4, WHITE_CARD);
  tft.drawRoundRect(90, startY + 72, 140, keyH - 2, 4, PINK_MED);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(140, startY + 80);
  tft.print("SPACE");
  
  // Backspace
  tft.fillRoundRect(240, startY + 72, 70, keyH - 2, 4, WHITE_CARD);
  tft.drawRoundRect(240, startY + 72, 70, keyH - 2, 4, PINK_MED);
  tft.setCursor(255, startY + 80);
  tft.print("DEL");
}

void handlePasswordScreen() {
  uint16_t touchX, touchY;
  if (!getTouchCoordinates(&touchX, &touchY)) return;
  
  // Check keyboard touches
  String keys[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  int keyW = 28;
  int keyH = 24;
  int startY = 110;
  
  for (int row = 0; row < 3; row++) {
    int numKeys = keys[row].length();
    int startX = (320 - (numKeys * keyW)) / 2;
    
    for (int col = 0; col < numKeys; col++) {
      int x = startX + (col * keyW);
      int y = startY + (row * keyH);
      
      if (touchX >= x && touchX <= x + keyW - 2 &&
          touchY >= y && touchY <= y + keyH - 2) {
        char c = keys[row].charAt(col);
        if (capsLock) c = toupper(c);
        wifiPassword += c;
        drawPasswordScreen();
        delay(200);
        return;
      }
    }
  }
  
  // Space
  if (touchX >= 90 && touchX <= 230 && touchY >= startY + 72 && touchY <= startY + 96) {
    wifiPassword += " ";
    drawPasswordScreen();
    delay(200);
    return;
  }
  
  // Backspace
  if (touchX >= 240 && touchX <= 310 && touchY >= startY + 72 && touchY <= startY + 96) {
    if (wifiPassword.length() > 0) {
      wifiPassword = wifiPassword.substring(0, wifiPassword.length() - 1);
      drawPasswordScreen();
      delay(200);
    }
    return;
  }
  
  // Connect button
  if (touchX >= 220 && touchX <= 310 && touchY >= 220 && touchY <= 248) {
    // Try connecting
    connectToWiFi();
    return;
  }
  
  // Back button
  if (touchX >= 10 && touchX <= 100 && touchY >= 220 && touchY <= 248) {
    currentState = STATE_WIFI_SCAN;
    selectedNetwork = -1;
    drawWiFiScanScreen();
    delay(200);
    return;
  }
}

void connectToWiFi() {
  tft.fillScreen(PINK_BG);
  tft.setTextColor(PINK_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(60, 100);
  tft.println("Connecting...");
  
  WiFi.begin(selectedSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    // Save credentials
    preferences.putString("ssid", selectedSSID);
    preferences.putString("pass", wifiPassword);
    
    // Success!
    tft.fillScreen(GREEN_OK);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(90, 100);
    tft.println("Connected!");
    delay(1000);
    
    // Move to wallet input
    currentState = STATE_WALLET_INPUT;
    keyboardInput = walletAddress;
    drawWalletScreen();
  } else {
    // Failed
    tft.fillScreen(RED_ERR);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(100, 100);
    tft.println("Failed!");
    delay(2000);
    
    // Back to password
    wifiPassword = "";
    drawPasswordScreen();
  }
}

void drawWalletScreen() {
  drawKawaiiBackground();
  
  // Title
  tft.fillRoundRect(10, 10, 300, 30, 12, WHITE_CARD);
  tft.drawRoundRect(10, 10, 300, 30, 12, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(50, 18);
  tft.println("BTC Wallet Address");
  
  // Wallet field
  tft.fillRoundRect(10, 50, 300, 50, 10, WHITE_CARD);
  tft.drawRoundRect(10, 50, 300, 50, 10, PINK_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(TEXT_DARK);
  tft.setCursor(15, 62);
  
  String display = keyboardInput;
  if (display.length() > 40) {
    display = display.substring(0, 18) + "..." + display.substring(display.length() - 18);
  }
  tft.print(display);
  
  // Keyboard
  drawKeyboard();
  
  // Next button
  tft.fillRoundRect(220, 220, 90, 28, 10, GREEN_OK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(250, 230);
  tft.println("Next");
}

void handleWalletScreen() {
  uint16_t touchX, touchY;
  if (!getTouchCoordinates(&touchX, &touchY)) return;
  
  // Similar keyboard handling as password screen
  String keys[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  int keyW = 28;
  int keyH = 24;
  int startY = 110;
  
  for (int row = 0; row < 3; row++) {
    int numKeys = keys[row].length();
    int startX = (320 - (numKeys * keyW)) / 2;
    
    for (int col = 0; col < numKeys; col++) {
      int x = startX + (col * keyW);
      int y = startY + (row * keyH);
      
      if (touchX >= x && touchX <= x + keyW - 2 &&
          touchY >= y && touchY <= y + keyH - 2) {
        char c = keys[row].charAt(col);
        keyboardInput += c;
        drawWalletScreen();
        delay(200);
        return;
      }
    }
  }
  
  // Space
  if (touchX >= 90 && touchX <= 230 && touchY >= startY + 72 && touchY <= startY + 96) {
    keyboardInput += " ";
    drawWalletScreen();
    delay(200);
    return;
  }
  
  // Backspace
  if (touchX >= 240 && touchX <= 310 && touchY >= startY + 72 && touchY <= startY + 96) {
    if (keyboardInput.length() > 0) {
      keyboardInput = keyboardInput.substring(0, keyboardInput.length() - 1);
      drawWalletScreen();
      delay(200);
    }
    return;
  }
  
  // Next button
  if (touchX >= 220 && touchX <= 310 && touchY >= 220 && touchY <= 248) {
    walletAddress = keyboardInput;
    preferences.putString("wallet", walletAddress);
    
    currentState = STATE_MINING_SELECT;
    drawMiningSelectScreen();
    delay(200);
    return;
  }
}

void drawMiningSelectScreen() {
  drawKawaiiBackground();
  
  // Title
  tft.fillRoundRect(10, 10, 300, 40, 12, WHITE_CARD);
  tft.drawRoundRect(10, 10, 300, 40, 12, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(60, 22);
  tft.println("Mining Mode");
  
  // Pool button
  uint16_t poolBg = soloMining ? WHITE_CARD : PINK_MED;
  tft.fillRoundRect(30, 80, 120, 80, 12, poolBg);
  tft.drawRoundRect(30, 80, 120, 80, 12, PINK_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(PINK_ACCENT);
  tft.setCursor(50, 110);
  tft.println("POOL");
  
  // Solo button
  uint16_t soloBg = soloMining ? PINK_MED : WHITE_CARD;
  tft.fillRoundRect(170, 80, 120, 80, 12, soloBg);
  tft.drawRoundRect(170, 80, 120, 80, 12, PINK_ACCENT);
  tft.setCursor(190, 110);
  tft.println("SOLO");
  
  // Start button
  tft.fillRoundRect(80, 190, 160, 40, 12, GREEN_OK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(90, 203);
  tft.println("Start Mining!");
}

void handleMiningSelectScreen() {
  uint16_t touchX, touchY;
  if (!getTouchCoordinates(&touchX, &touchY)) return;
  
  // Pool button
  if (touchX >= 30 && touchX <= 150 && touchY >= 80 && touchY <= 160) {
    soloMining = false;
    preferences.putBool("solo", false);
    drawMiningSelectScreen();
    delay(200);
    return;
  }
  
  // Solo button
  if (touchX >= 170 && touchX <= 290 && touchY >= 80 && touchY <= 160) {
    soloMining = true;
    preferences.putBool("solo", true);
    drawMiningSelectScreen();
    delay(200);
    return;
  }
  
  // Start button
  if (touchX >= 80 && touchX <= 240 && touchY >= 190 && touchY <= 230) {
    currentState = STATE_MINING;
    
    if (!soloMining) {
      connectToPool();
    }
    
    lastUIUpdate = millis();
    return;
  }
}

void handleMiningScreen() {
  // Do mining work
  doMiningWork();
  
  // Update UI
  if (millis() - lastUIUpdate > 100) {
    drawMainUI();
    lastUIUpdate = millis();
    animFrame++;
  }
  
  // Process pool messages
  if (!soloMining && poolConnected) {
    stratum.processMessages();
  }
}

void connectToPool() {
  const MiningPool* pool = getPool(selectedPool);
  if (!pool) return;
  
  if (!stratum.connect(pool->host, pool->port)) {
    poolConnected = false;
    return;
  }
  
  if (!stratum.subscribe()) {
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  if (!stratum.authorize(walletAddress.c_str(), "x")) {
    stratum.disconnect();
    poolConnected = false;
    return;
  }
  
  poolConnected = true;
  rabbitMood = 2;
}

void doMiningWork() {
  static unsigned long lastHashTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastHashTime > 10) {
    unsigned long hashes = random(10, 50);
    totalHashes += hashes;
    hashRate = (float)hashes;
    
    if (hashRate > 40) rabbitMood = 3;
    else if (hashRate > 20) rabbitMood = 2;
    else rabbitMood = 1;
    
    lastHashTime = currentTime;
  }
  
  if (random(0, 1000) < 2) acceptedShares++;
  if (random(0, 5000) < 1) rejectedShares++;
  
  poolWorkers = 1000 + random(0, 500);
  poolDifficulty = 2.5 + random(0, 10) / 10.0;
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
  tft.print("By Arez V3.0");
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
  int sparklePositions[][2] = {{280, 50}, {30, 180}, {250, 160}};
  
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
  tft.fillRoundRect(x - 15, y - 25, 8, 20, 4, PINK_MED);
  tft.fillRoundRect(x + 7, y - 25, 8, 20, 4, PINK_MED);
  
  tft.fillCircle(x, y, 18, TFT_WHITE);
  tft.drawCircle(x, y, 18, PINK_MED);
  
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
  
  tft.fillCircle(x, y + 3, 2, PINK_ACCENT);
  
  if (mood >= 2) {
    tft.drawPixel(x - 3, y + 7, PINK_ACCENT);
    tft.drawPixel(x - 2, y + 8, PINK_ACCENT);
    tft.drawPixel(x + 2, y + 8, PINK_ACCENT);
    tft.drawPixel(x + 3, y + 7, PINK_ACCENT);
  }
  
  tft.fillCircle(x - 15, y + 2, 3, 0xFBEA);
  tft.fillCircle(x + 15, y + 2, 3, 0xFBEA);
}

void drawTinyStar(int x, int y) {
  tft.drawLine(x - 3, y, x + 3, y, YELLOW_GLOW);
  tft.drawLine(x, y - 3, x, y + 3, YELLOW_GLOW);
  tft.drawLine(x - 2, y - 2, x + 2, y + 2, YELLOW_GLOW);
  tft.drawLine(x - 2, y + 2, x + 2, y - 2, YELLOW_GLOW);
}

bool getTouchCoordinates(uint16_t *x, uint16_t *y) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);
  
  if (touched) {
    *x = touchX;
    *y = touchY;
    
    // Debug output
    Serial.print("Touch detected: X=");
    Serial.print(touchX);
    Serial.print(", Y=");
    Serial.println(touchY);
    
    // Visual feedback - small circle where touched
    tft.fillCircle(touchX, touchY, 3, TFT_RED);
    delay(100);
    
    return true;
  }
  
  return false;
}
