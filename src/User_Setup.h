// User_Setup.h for ESP32-2432S028 CYD Display
// This file configures TFT_eSPI for the Cheap Yellow Display

#ifndef USER_SETUP_H
#define USER_SETUP_H

#define USER_SETUP_INFO "ESP32_CYD_2432S028"
#define USER_SETUP_LOADED

// Driver
#define ILI9341_DRIVER

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ESP32 CYD Pin Configuration
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1  // Not connected

// Touch screen
#define TOUCH_CS 33

// Fonts to load
#define LOAD_GLCD   // Font 1
#define LOAD_FONT2  // Font 2
#define LOAD_FONT4  // Font 4
#define LOAD_FONT6  // Font 6
#define LOAD_FONT7  // Font 7
#define LOAD_FONT8  // Font 8
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// SPI Frequencies
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

#endif // USER_SETUP_H
