#include "OTAUpdater.h"
#include <WiFi.h>       // Pro použití WiFiClient
#include <Update.h>     // Pro OTA aktualizace
#include <HTTPClient.h> // Pro HTTP požadavky
#include <ArduinoJson.h>

const char* versionURL = "https://api.djdevs.eu/firmware/josef/version.json";

void OTAUpdater::checkForUpdates(const String& currentVersion) {
  if (!upToDate(currentVersion)) {
    Serial.println("Nová verze dostupná! Spouštím aktualizaci...");

    // Znovu stáhneme JSON a provedeme aktualizaci
    HTTPClient http;
    http.begin(versionURL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      StaticJsonDocument<256> jsonDoc;
      if (deserializeJson(jsonDoc, payload) == DeserializationError::Ok) {
        const char* downloadURL = jsonDoc["download"];
        OTAUpdater::performOTAUpdate(downloadURL);
      }
    }
    http.end();
  } else {
    Serial.println("Zařízení je aktuální.");
  }
}

bool OTAUpdater::upToDate(const String& currentVersion) {
  HTTPClient http;
  http.begin(versionURL);
  int httpCode = http.GET();
  bool isUpToDate = true; // Výchozí hodnota - předpokládáme, že je aktuální.

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Kontrola aktualizace JSON:");
    Serial.println(payload);

    StaticJsonDocument<256> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, payload);

    if (!error) {
      String newVersion = jsonDoc["version"].as<String>();
      bool newVersionForce = jsonDoc["force"].as<bool>();
      Serial.printf("Aktuální verze: %s, Dostupná verze: %s\n", currentVersion.c_str(), newVersion.c_str());

      if (newVersion != currentVersion) {
        isUpToDate = false;
        if(newVersionForce){
          OTAUpdater::checkForUpdates(currentVersion);
        }
      }
    } else {
      Serial.println("Chyba při parsování JSON.");
    }
  } else {
    Serial.printf("Chyba při přístupu na URL: %s, HTTP kód: %d\n", versionURL, httpCode);
  }
  http.end();
  return isUpToDate;
}

void OTAUpdater::performOTAUpdate(const char* downloadURL) {
  HTTPClient http;
  http.begin(downloadURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    WiFiClient& client = http.getStream();

    if (Update.begin(contentLength)) {
      Serial.println("Začíná OTA aktualizace...");

      int written = 0;
      int prevProgress = 0;
      uint8_t buffer[1024];

      while (client.connected() && written < contentLength) {
        size_t bytesAvailable = client.available();
        if (bytesAvailable > 0) {
          int bytesRead = client.readBytes(buffer, min((size_t)bytesAvailable, sizeof(buffer)));
          if (bytesRead > 0) {
            size_t bytesWritten = Update.write(buffer, bytesRead);
            if (bytesWritten > 0) {
              written += bytesWritten;

              int progress = (written * 100) / contentLength;
              if (progress != prevProgress) {
                Serial.printf("Progres: %d%%\n", progress);
                prevProgress = progress;
              }
            } else {
              Serial.println("Chyba při zápisu dat!");
              break;
            }
          }
        }
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("OTA aktualizace byla úspěšná!");
          Serial.println("Restart zařízení...");
          ESP.restart();
        } else {
          Serial.println("Aktualizace nedokončena.");
        }
      } else {
        Serial.printf("Chyba při ukončení aktualizace: %s\n", Update.errorString());
      }
    } else {
      Serial.println("Není dostatek místa pro OTA update.");
    }
  } else {
    Serial.printf("Chyba při stahování binárního souboru, HTTP kód: %d\n", httpCode);
  }
  http.end();
}
