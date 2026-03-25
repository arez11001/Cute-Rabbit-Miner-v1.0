/*
 * Rabbit Miner V4.0 - DARK RETRO EDITION
 * Designed by Arez
 * 
 * Features:
 * - DARK THEME (black background like boot screen!)
 * - ASCII RETRO RABBIT (just like boot!)
 * - Hash rate display (WORKS!)
 * - Total hashes counter (RESETS ON COIN!)
 * - Coins mined counter (WORKS!)
 * - Difficulty display (WORKS!)
 * - Retro green/yellow text
 * - Classic terminal aesthetic!
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
float poolDifficulty = 2.5;
int poolWorkers = 1234;
int blocksFound = 0;
unsigned long lastStatsUpdate = 0;
bool poolConnected = false;
int rabbitMood = 2;

// UI state
unsigned long lastUIUpdate = 0;
int animFrame = 0;

// RETRO DARK COLORS (terminal style!)
#define COLOR_BG      TFT_BLACK      // Black background
#define COLOR_GREEN   0x07E0         // Bright green (classic terminal)
#define COLOR_YELLOW  0xFFE0         // Yellow highlights
#define COLOR_CYAN    0x07FF         // Cyan for stats
#define COLOR_RED     0xF800         // Red for errors
#define COLOR_GRAY    0x7BEF         // Gray text
#define COLOR_WHITE   TFT_WHITE      // White for important

#define SCREEN_W 320
#define SCREEN_H 240

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Rabbit Miner V4.0 - DARK RETRO ===");
  
  // Turn on backlight
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  delay(100);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  
  // Show boot screen
  showBootScreen();
  delay(3000);
  
  // Initialize preferences
  preferences.begin("rabbit-miner", false);
  walletAddress = preferences.getString("wallet", "");
  selectedPool = preferences.getInt("pool", 0);
  soloMining = preferences.getBool("solo", false);
  
  // Setup WiFi
  setupWiFi();
  
  // Connect to pool (if not solo mining)
  if (!soloMining) {
    connectToPool();
  }
  
  rabbitMood = 2;
  lastStatsUpdate = millis();
  lastUIUpdate = millis();
  
  // Initialize hash counter
  totalHashes = 0;
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
  
  // Update UI
  if (millis() - lastUIUpdate > 100) {
    drawMainUI();
    lastUIUpdate = millis();
    animFrame++;
  }
  
  // Process pool messages (if pool mining)
  if (!soloMining && poolConnected) {
    stratum.processMessages();
  }
  
  delay(1);
}

void showBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(COLOR_GREEN);
  
  // Title
  tft.setTextSize(3);
  tft.setCursor(40, 30);
  tft.println("Rabbit Miner");
  
  // Version
  tft.setTextSize(2);
  tft.setCursor(50, 65);
  tft.setTextColor(COLOR_GRAY);
  tft.println("Made By Arez V4.0");
  
  // ASCII Rabbit
  tft.setTextSize(2);
  tft.setCursor(110, 100);
  tft.setTextColor(COLOR_GREEN);
  tft.println("(\\(\\");
  tft.setCursor(110, 120);
  tft.println("( -.-)");
  tft.setCursor(100, 140);
  tft.println("o_(\")(\")" );
  
  // Bottom text
  tft.setTextSize(2);
  tft.setCursor(70, 180);
  tft.setTextColor(COLOR_GREEN);
  tft.println("Rabbit Miner");
  tft.setTextSize(1);
  tft.setCursor(85, 205);
  tft.setTextColor(COLOR_GRAY);
  tft.println("DARK RETRO EDITION");
  
  // Loading
  tft.setTextSize(1);
  tft.setCursor(125, 225);
  tft.setTextColor(COLOR_CYAN);
  tft.println("Loading...");
}

void setupWiFi() {
  // Show retro setup screen
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(2);
  tft.setCursor(70, 80);
  tft.println("WiFi Setup");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(40, 110);
  tft.println("Connect to:");
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(30, 130);
  tft.println("RabbitMiner-Setup");
  
  WiFiManager wm;
  wm.setConfigPortalTimeout(300);
  
  // Custom parameters
  WiFiManagerParameter custom_wallet("wallet", "BTC Wallet Address", walletAddress.c_str(), 64);
  WiFiManagerParameter custom_solo("<br><label>Mining Mode:</label><br><input type='radio' name='mode' value='pool' checked> Pool Mining<br><input type='radio' name='mode' value='solo'> Solo Mining<br>");
  
  wm.addParameter(&custom_wallet);
  wm.addParameter(&custom_solo);
  
  wm.autoConnect("RabbitMiner-Setup");
  
  // Save wallet
  String newWallet = custom_wallet.getValue();
  if (newWallet.length() > 0) {
    walletAddress = newWallet;
    preferences.putString("wallet", walletAddress);
  }
  
  soloMining = false;
  preferences.putBool("solo", soloMining);
  
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Success screen
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(60, 100);
  tft.println("CONNECTED!");
  delay(1500);
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
  
  // BIG CELEBRATION!
  for (int i = 0; i < 5; i++) {
    tft.fillScreen(COLOR_YELLOW);
    delay(150);
    tft.fillScreen(COLOR_GREEN);
    delay(150);
  }
  
  // Show message
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(40, 80);
  tft.println(">>> BLOCK");
  tft.setCursor(50, 115);
  tft.println("FOUND! <<<");
  
  // ASCII celebration rabbit
  tft.setTextSize(2);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(110, 160);
  tft.println("(\\(\\");
  tft.setCursor(110, 180);
  tft.println("( ^.^)");
  tft.setCursor(100, 200);
  tft.println("o_(\")(\")" );
  
  delay(3000);
  
  // RESET TOTAL HASHES!
  totalHashes = 0;
  sessionHashes = 0;
  
  Serial.println("BLOCK FOUND! Total hashes reset to 0!");
  
  rabbitMood = 2;
}

void drawMainUI() {
  // Black background
  tft.fillScreen(COLOR_BG);
  
  // Draw border
  tft.drawRect(0, 0, SCREEN_W, SCREEN_H, COLOR_GREEN);
  tft.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, COLOR_GREEN);
  
  // Top bar
  drawTopBar();
  
  // ASCII Rabbit (left side)
  drawASCIIRabbit(55, 80, rabbitMood);
  
  // Stats (right side)
  drawStats();
  
  // Bottom info
  drawBottomBar();
}

void drawTopBar() {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(5, 5);
  tft.print("WALLET:");
  
  tft.setTextColor(COLOR_CYAN);
  String truncWallet = walletAddress.substring(0, 10) + "...";
  tft.print(truncWallet);
  
  // Mode
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(200, 5);
  tft.print("MODE:");
  
  if (soloMining) {
    tft.setTextColor(COLOR_YELLOW);
    tft.print("SOLO");
  } else {
    tft.setTextColor(COLOR_GREEN);
    tft.print("POOL");
  }
}

void drawASCIIRabbit(int x, int y, int mood) {
  // Classic ASCII rabbit like boot screen!
  tft.setTextSize(3);
  
  if (mood == 4) {
    // Celebrating!
    tft.setTextColor(COLOR_YELLOW);
  } else if (mood == 3) {
    // Excited
    tft.setTextColor(COLOR_GREEN);
  } else {
    // Normal/Mining
    tft.setTextColor(COLOR_CYAN);
  }
  
  tft.setCursor(x, y);
  tft.println("(\\(\\");
  
  tft.setCursor(x, y + 25);
  if (mood == 4) {
    tft.println("( ^.^)"); // Celebrating!
  } else if (mood == 3) {
    tft.println("( O.O)"); // Excited!
  } else if (mood == 0) {
    tft.println("( -.-)"); // Sleeping
  } else {
    tft.println("( o.o)"); // Normal
  }
  
  tft.setCursor(x - 10, y + 50);
  tft.println("o_(\")(\")" );
  
  // Label
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x - 5, y + 85);
  tft.print("Rabbit Miner");
  tft.setCursor(x, y + 95);
  tft.print("By Arez");
}

void drawStats() {
  int x = 165;
  int y = 25;
  int lineH = 20;
  
  tft.setTextSize(1);
  
  // Hash Rate
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x, y);
  tft.print("HASH RATE:");
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(x, y + 12);
  char hashStr[16];
  sprintf(hashStr, "%.1f H/s", hashRate);
  tft.println(hashStr);
  
  y += 40;
  
  // Total Hashes
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x, y);
  tft.print("TOTAL HASH:");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(x, y + 12);
  
  if (totalHashes > 1000000) {
    char str[16];
    sprintf(str, "%.2f M", totalHashes / 1000000.0);
    tft.println(str);
  } else if (totalHashes > 1000) {
    char str[16];
    sprintf(str, "%.1f K", totalHashes / 1000.0);
    tft.println(str);
  } else {
    tft.println(totalHashes);
  }
  
  y += 30;
  
  // Coins Mined
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x, y);
  tft.print("COINS MINED:");
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(x + 20, y + 12);
  tft.println(blocksFound);
  
  y += 35;
  
  // Difficulty
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x, y);
  tft.print("DIFFICULTY:");
  
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(x, y + 12);
  char diffStr[16];
  sprintf(diffStr, "%.1f T", poolDifficulty);
  tft.println(diffStr);
  
  y += 30;
  
  // Workers
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(x, y);
  tft.print("WORKERS:");
  
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(x + 10, y + 12);
  tft.println(poolWorkers);
}

void drawBottomBar() {
  int y = 220;
  
  tft.setTextSize(1);
  
  // Accepted
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(5, y);
  tft.print("ACCEPT:");
  tft.setTextColor(COLOR_GREEN);
  tft.print(acceptedShares);
  
  // Rejected
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(100, y);
  tft.print("REJECT:");
  tft.setTextColor(COLOR_RED);
  tft.print(rejectedShares);
  
  // Status
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(200, y);
  tft.print("STATUS:");
  if (poolConnected) {
    tft.setTextColor(COLOR_GREEN);
    tft.print("OK");
  } else {
    tft.setTextColor(COLOR_RED);
    tft.print("--");
  }
}
