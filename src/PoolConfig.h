/*
 * PoolConfig.h - Mining Pool Configurations
 * For Rabbit Miner ESP32 CYD
 */

#ifndef POOL_CONFIG_H
#define POOL_CONFIG_H

// Pool configuration structure
struct MiningPool {
  const char* name;
  const char* host;
  int port;
  const char* description;
};

// Popular Bitcoin mining pools
const MiningPool MINING_POOLS[] = {
  {
    "Slush Pool",
    "stratum.slushpool.com",
    3333,
    "Oldest Bitcoin mining pool, est. 2010"
  },
  {
    "Antpool",
    "stratum.antpool.com",
    3333,
    "Large pool operated by Bitmain"
  },
  {
    "F2Pool",
    "btc.f2pool.com",
    3333,
    "Chinese mining pool, multicurrency"
  },
  {
    "ViaBTC",
    "btc.viabtc.com",
    3333,
    "Full PPS+ pool with low fees"
  },
  {
    "BTC.com",
    "stratum.btc.com",
    3333,
    "User-friendly pool with good stats"
  },
  {
    "Poolin",
    "btc.ss.poolin.me",
    443,
    "Multi-currency pool"
  },
  {
    "Solo CK",
    "solo.ckpool.org",
    3333,
    "Solo mining pool (for fun!)"
  }
};

const int NUM_POOLS = sizeof(MINING_POOLS) / sizeof(MiningPool);

// Helper function to get pool by index
const MiningPool* getPool(int index) {
  if (index >= 0 && index < NUM_POOLS) {
    return &MINING_POOLS[index];
  }
  return nullptr;
}

// Helper function to find pool by name
const MiningPool* findPool(const char* name) {
  for (int i = 0; i < NUM_POOLS; i++) {
    if (strcmp(MINING_POOLS[i].name, name) == 0) {
      return &MINING_POOLS[i];
    }
  }
  return nullptr;
}

#endif // POOL_CONFIG_H
