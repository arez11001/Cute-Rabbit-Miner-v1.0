/*
 * StratumClient.h - Bitcoin Stratum Pool Protocol Client
 * For Rabbit Miner ESP32 CYD
 */

#ifndef STRATUM_CLIENT_H
#define STRATUM_CLIENT_H

#include <WiFiClient.h>
#include <ArduinoJson.h>

class StratumClient {
public:
  StratumClient();
  bool connect(const char* host, int port);
  bool subscribe();
  bool authorize(const char* user, const char* pass);
  bool getWork(String& jobId, String& prevHash, String& coinb1, String& coinb2, 
               String& merkleRoot, String& version, String& nBits, String& nTime);
  bool submitShare(const char* jobId, const char* nonce, const char* nTime);
  void processMessages();
  bool isConnected();
  void disconnect();
  
  // Callbacks
  void onNewBlock(void (*callback)());
  void onDifficulty(void (*callback)(float));
  
private:
  WiFiClient client;
  int messageId;
  bool connected;
  String extraNonce1;
  int extraNonce2Size;
  
  void (*newBlockCallback)();
  void (*difficultyCallback)(float);
  
  bool sendRequest(const char* method, JsonDocument& params);
  bool readResponse(JsonDocument& response);
  String generateRequestId();
};

StratumClient::StratumClient() {
  messageId = 1;
  connected = false;
  newBlockCallback = nullptr;
  difficultyCallback = nullptr;
}

bool StratumClient::connect(const char* host, int port) {
  Serial.printf("Connecting to pool %s:%d\n", host, port);
  
  if (client.connect(host, port)) {
    Serial.println("Pool connection established");
    connected = true;
    return true;
  }
  
  Serial.println("Pool connection failed");
  connected = false;
  return false;
}

bool StratumClient::subscribe() {
  if (!connected) return false;
  
  StaticJsonDocument<512> params;
  params.add("RabbitMiner/1.0");
  
  if (!sendRequest("mining.subscribe", params)) {
    return false;
  }
  
  StaticJsonDocument<1024> response;
  if (!readResponse(response)) {
    return false;
  }
  
  // Extract extra nonce info
  if (response.containsKey("result")) {
    JsonArray result = response["result"];
    if (result.size() >= 2) {
      extraNonce1 = result[1].as<String>();
      extraNonce2Size = result[2].as<int>();
      Serial.printf("Subscribed: ExtraNonce1=%s, Size=%d\n", 
                    extraNonce1.c_str(), extraNonce2Size);
      return true;
    }
  }
  
  return false;
}

bool StratumClient::authorize(const char* user, const char* pass) {
  if (!connected) return false;
  
  StaticJsonDocument<512> params;
  params.add(user);
  params.add(pass);
  
  if (!sendRequest("mining.authorize", params)) {
    return false;
  }
  
  StaticJsonDocument<512> response;
  if (!readResponse(response)) {
    return false;
  }
  
  if (response.containsKey("result")) {
    bool authorized = response["result"].as<bool>();
    Serial.printf("Authorization: %s\n", authorized ? "Success" : "Failed");
    return authorized;
  }
  
  return false;
}

bool StratumClient::getWork(String& jobId, String& prevHash, String& coinb1, 
                            String& coinb2, String& merkleRoot, String& version, 
                            String& nBits, String& nTime) {
  // Work is received via mining.notify method
  // This is a placeholder - actual implementation would store work from notifications
  return false;
}

bool StratumClient::submitShare(const char* jobId, const char* nonce, const char* nTime) {
  if (!connected) return false;
  
  StaticJsonDocument<512> params;
  params.add("RabbitMiner");
  params.add(jobId);
  params.add(extraNonce1);
  params.add(nonce);
  params.add(nTime);
  
  if (!sendRequest("mining.submit", params)) {
    return false;
  }
  
  StaticJsonDocument<512> response;
  if (!readResponse(response)) {
    return false;
  }
  
  if (response.containsKey("result")) {
    bool accepted = response["result"].as<bool>();
    Serial.printf("Share submitted: %s\n", accepted ? "Accepted" : "Rejected");
    return accepted;
  }
  
  return false;
}

void StratumClient::processMessages() {
  if (!connected || !client.available()) return;
  
  StaticJsonDocument<2048> doc;
  String line = client.readStringUntil('\n');
  
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Check for mining.notify (new work)
  if (doc.containsKey("method")) {
    String method = doc["method"].as<String>();
    
    if (method == "mining.notify") {
      Serial.println("New work received");
      if (newBlockCallback) {
        newBlockCallback();
      }
    }
    else if (method == "mining.set_difficulty") {
      if (doc.containsKey("params")) {
        float difficulty = doc["params"][0].as<float>();
        Serial.printf("Difficulty set to: %.2f\n", difficulty);
        if (difficultyCallback) {
          difficultyCallback(difficulty);
        }
      }
    }
  }
}

bool StratumClient::isConnected() {
  return connected && client.connected();
}

void StratumClient::disconnect() {
  if (client.connected()) {
    client.stop();
  }
  connected = false;
}

void StratumClient::onNewBlock(void (*callback)()) {
  newBlockCallback = callback;
}

void StratumClient::onDifficulty(void (*callback)(float)) {
  difficultyCallback = callback;
}

bool StratumClient::sendRequest(const char* method, JsonDocument& params) {
  StaticJsonDocument<1024> request;
  request["id"] = messageId++;
  request["method"] = method;
  request["params"] = params;
  
  String output;
  serializeJson(request, output);
  output += "\n";
  
  Serial.printf("Sending: %s", output.c_str());
  client.print(output);
  
  return true;
}

bool StratumClient::readResponse(JsonDocument& response) {
  unsigned long timeout = millis() + 5000;
  
  while (!client.available()) {
    if (millis() > timeout) {
      Serial.println("Response timeout");
      return false;
    }
    delay(10);
  }
  
  String line = client.readStringUntil('\n');
  Serial.printf("Received: %s\n", line.c_str());
  
  DeserializationError error = deserializeJson(response, line);
  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return false;
  }
  
  return true;
}

#endif // STRATUM_CLIENT_H
