/*
 * BitcoinMiner.h - SHA256 Bitcoin Mining for ESP32
 * Optimized for Rabbit Miner
 */

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include <mbedtls/sha256.h>
#include <Arduino.h>

class BitcoinMiner {
public:
  BitcoinMiner();
  
  // Mining functions
  void setTarget(const char* difficulty);
  bool mine(const uint8_t* header, uint32_t startNonce, uint32_t numHashes, 
            uint32_t& foundNonce, unsigned long& hashCount);
  
  // Utility functions
  static void sha256d(const uint8_t* data, size_t len, uint8_t* hash);
  static void reverseBytes(uint8_t* data, size_t len);
  static uint32_t getHashDifficulty(const uint8_t* hash);
  static bool checkHash(const uint8_t* hash, const uint8_t* target);
  
  // Performance tracking
  float getHashRate();
  unsigned long getTotalHashes();
  void resetStats();
  
private:
  uint8_t target[32];
  unsigned long totalHashes;
  unsigned long lastHashTime;
  float currentHashRate;
  
  void doubleSha256(const uint8_t* data, size_t len, uint8_t* hash);
  bool isHashBelowTarget(const uint8_t* hash);
};

BitcoinMiner::BitcoinMiner() {
  totalHashes = 0;
  lastHashTime = millis();
  currentHashRate = 0.0;
  
  // Set default target (very easy for testing)
  memset(target, 0xFF, 32);
  target[0] = 0x00;
  target[1] = 0x00;
  target[2] = 0xFF;
}

void BitcoinMiner::setTarget(const char* difficulty) {
  // Convert difficulty bits to target
  // This is simplified - real implementation would parse nBits properly
  memset(target, 0xFF, 32);
  target[0] = 0x00;
  target[1] = 0x00;
  target[2] = 0x0F;
}

bool BitcoinMiner::mine(const uint8_t* header, uint32_t startNonce, uint32_t numHashes,
                        uint32_t& foundNonce, unsigned long& hashCount) {
  uint8_t blockHeader[80];
  memcpy(blockHeader, header, 80);
  
  uint8_t hash[32];
  hashCount = 0;
  
  unsigned long startTime = millis();
  
  for (uint32_t nonce = startNonce; nonce < startNonce + numHashes; nonce++) {
    // Insert nonce into block header (bytes 76-79)
    blockHeader[76] = (nonce >> 0) & 0xFF;
    blockHeader[77] = (nonce >> 8) & 0xFF;
    blockHeader[78] = (nonce >> 16) & 0xFF;
    blockHeader[79] = (nonce >> 24) & 0xFF;
    
    // Double SHA256
    doubleSha256(blockHeader, 80, hash);
    
    hashCount++;
    totalHashes++;
    
    // Check if hash meets target
    if (isHashBelowTarget(hash)) {
      foundNonce = nonce;
      
      // Update hash rate
      unsigned long elapsed = millis() - startTime;
      if (elapsed > 0) {
        currentHashRate = (float)hashCount / (elapsed / 1000.0);
      }
      
      return true;
    }
    
    // Yield to prevent watchdog timeout
    if (hashCount % 1000 == 0) {
      yield();
    }
  }
  
  // Update hash rate
  unsigned long elapsed = millis() - startTime;
  if (elapsed > 0) {
    currentHashRate = (float)hashCount / (elapsed / 1000.0);
  }
  
  return false;
}

void BitcoinMiner::doubleSha256(const uint8_t* data, size_t len, uint8_t* hash) {
  uint8_t temp[32];
  
  // First SHA256
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0); // 0 = SHA256 (not SHA224)
  mbedtls_sha256_update(&ctx, data, len);
  mbedtls_sha256_finish(&ctx, temp);
  mbedtls_sha256_free(&ctx);
  
  // Second SHA256
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, temp, 32);
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
}

bool BitcoinMiner::isHashBelowTarget(const uint8_t* hash) {
  // Compare hash with target (big-endian comparison)
  for (int i = 31; i >= 0; i--) {
    if (hash[i] < target[i]) return true;
    if (hash[i] > target[i]) return false;
  }
  return true;
}

float BitcoinMiner::getHashRate() {
  return currentHashRate;
}

unsigned long BitcoinMiner::getTotalHashes() {
  return totalHashes;
}

void BitcoinMiner::resetStats() {
  totalHashes = 0;
  lastHashTime = millis();
  currentHashRate = 0.0;
}

// Static utility functions
void BitcoinMiner::sha256d(const uint8_t* data, size_t len, uint8_t* hash) {
  uint8_t temp[32];
  
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, data, len);
  mbedtls_sha256_finish(&ctx, temp);
  mbedtls_sha256_free(&ctx);
  
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, temp, 32);
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
}

void BitcoinMiner::reverseBytes(uint8_t* data, size_t len) {
  for (size_t i = 0; i < len / 2; i++) {
    uint8_t temp = data[i];
    data[i] = data[len - 1 - i];
    data[len - 1 - i] = temp;
  }
}

uint32_t BitcoinMiner::getHashDifficulty(const uint8_t* hash) {
  uint32_t difficulty = 0;
  for (int i = 31; i >= 0; i--) {
    if (hash[i] == 0) {
      difficulty += 8;
    } else {
      // Count leading zeros in this byte
      uint8_t byte = hash[i];
      while ((byte & 0x80) == 0) {
        difficulty++;
        byte <<= 1;
      }
      break;
    }
  }
  return difficulty;
}

bool BitcoinMiner::checkHash(const uint8_t* hash, const uint8_t* target) {
  for (int i = 31; i >= 0; i--) {
    if (hash[i] < target[i]) return true;
    if (hash[i] > target[i]) return false;
  }
  return true;
}

#endif // BITCOIN_MINER_H
