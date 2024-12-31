#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
// OTA knihovny
#include "OTAUpdater.h"

const String currentVersion = "v2.0.06";

Preferences preferences;
WebServer server(80);

const char *apSSID = "ESP32_Radio";
const char *apPassword = "12345678";



String wifiSSID;
String wifiPassword;
String streamURL;

// Objekty pro přehrávání
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

void handleRoot() {
  String html = "<html><head><style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f9; color: #333; }";
  html += "h1 { background-color: #4CAF50; color: white; padding: 10px 20px; }";
  html += "form { margin: 20px; padding: 20px; background-color: white; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }";
  html += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 20px; cursor: pointer; border-radius: 4px; }";
  html += "input[type='submit']:hover { background-color: #45a049; }";
  html += "</style></head><body><h1>ESP32 Radio Config</h1>";
  html += "<form action='/save' method='POST'>";
  html += "<label>WiFi SSID:</label><input type='text' name='ssid' value='" + wifiSSID + "'><br>";
  html += "<label>WiFi Password:</label><input type='password' name='password' value='" + wifiPassword + "'><br>";
  html += "<label>Stream URL:</label><input type='text' name='url' value='" + streamURL + "'><br>";
  html += "<input type='submit' value='Save'></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("url")) {
    wifiSSID = server.arg("ssid");
    wifiPassword = server.arg("password");
    streamURL = server.arg("url");

    preferences.begin("wifi-config", false);
    preferences.putString("ssid", wifiSSID);
    preferences.putString("password", wifiPassword);
    preferences.putString("url", streamURL);
    preferences.end();

    server.send(200, "text/html", "<html><body><h1>Settings saved! Rebooting...</h1></body></html>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "<html><body><h1>Missing parameters!</h1></body></html>");
  }
}

void setupWiFi() {
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    // První kontrola aktualizace při startu
    OTAUpdater::checkForUpdates(currentVersion);
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void setupAP() {
  WiFi.softAP(apSSID, apPassword);
  IPAddress local_IP(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("HTTP server started.");
}

void setupPlayer() {
  file = new AudioFileSourceHTTPStream(streamURL.c_str());
buff = new AudioFileSourceBuffer(file, 65536);  // Dvojnásobná velikost

  // Nastavení I2S na interní DAC
  out = new AudioOutputI2S(0, 1);  // Použití interního DAC (GPIO 25 a GPIO 26)
  out->SetPinout(26, 25, -1);      // Pin 26 jako výstup (DAC1), -1 znamená nepoužití datového pinu
  out->SetGain(0.2);               // Nastavení hlasitosti

  mp3 = new AudioGeneratorMP3();
  mp3->begin(buff, out);
}

void setup() {
  Serial.begin(115200);

  preferences.begin("wifi-config", true);
    wifiSSID = "I'm Under The Studio";
  wifiPassword = "vlkov137";
  streamURL = "http://www.myinstants.com/media/sounds/la-cucaracha.mp3";

  // wifiSSID = preferences.getString("ssid", "");
  // wifiPassword = preferences.getString("password", "");
  // streamURL = preferences.getString("url", "http://ice.radia.cz/cernahora128.mp3");
  preferences.end();

  setupAP();

  if (!wifiSSID.isEmpty() && !wifiPassword.isEmpty()) {
    setupWiFi();

    if (WiFi.status() == WL_CONNECTED) {
      setupPlayer();
    }
  }
}

void loop() {
  server.handleClient();

  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("Stream error: Attempting to restart...");
      file->open(streamURL.c_str());  // Pokus o znovuotevření streamu
      mp3->begin(buff, out);
    }
  } else {
    delay(1000);  // Prodleva pro snížení zátěže v případě chyby
  }

  // Každou sekundu kontrolujeme stav WiFi
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime >= 10000) {
    lastCheckTime = millis();
      if (!OTAUpdater::upToDate(currentVersion)) {
        Serial.println("Nová verze detekována! Aktualizace dostupná.");
      }
  }
}
