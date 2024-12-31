#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Encoder.h>
#include <Arduino_I2S.h>
#include "radialist.h" // Váš JSON seznam rádií ve formátu, který jste uvedl

// Nastavení displeje
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Nastavení enkodéru
ESP32Encoder encoder;
#define ENCODER_BTN_PIN 15 // Pin pro tlačítko enkodéru

// Wi-Fi
Preferences preferences;
AsyncWebServer server(80);

const char* apSSID = "ESP32-Radio";
const char* apPassword = "12345678";
char wifiSSID[32] = "";
char wifiPassword[64] = "";

// URL rádia
String radioURL;
int selectedRadio = 0; // Výchozí výběr rádia

void connectToWiFi() {
  WiFi.begin(wifiSSID, wifiPassword);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    lcd.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AP Mode Active");
    WiFi.softAP(apSSID, apPassword);
    lcd.setCursor(0, 1);
    lcd.print("192.168.1.1");
  }
}

void savePreferences() {
  preferences.begin("wifi", false);
  preferences.putString("ssid", wifiSSID);
  preferences.putString("password", wifiPassword);
  preferences.putInt("radio", selectedRadio);
  preferences.end();
}

void loadPreferences() {
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  selectedRadio = preferences.getInt("radio", 0);
  preferences.end();

  ssid.toCharArray(wifiSSID, sizeof(wifiSSID));
  password.toCharArray(wifiPassword, sizeof(wifiPassword));
}

void handleForm(AsyncWebServerRequest *request) {
  if (request->hasParam("ssid") && request->hasParam("password") && request->hasParam("radio")) {
    String ssid = request->getParam("ssid")->value();
    String password = request->getParam("password")->value();
    selectedRadio = request->getParam("radio")->value().toInt();

    ssid.toCharArray(wifiSSID, sizeof(wifiSSID));
    password.toCharArray(wifiPassword, sizeof(wifiPassword));

    savePreferences();
    request->send(200, "text/plain", "Settings Saved. Reboot ESP32 to apply.");
  } else {
    request->send(400, "text/plain", "Missing Parameters");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <body>
        <h2>Wi-Fi & Radio Config</h2>
        <form action="/set" method="get">
          <label for="ssid">SSID:</label><br>
          <input type="text" id="ssid" name="ssid" value=""><br>
          <label for="password">Password:</label><br>
          <input type="text" id="password" name="password" value=""><br>
          <label for="radio">Radio Number:</label><br>
          <input type="number" id="radio" name="radio" min="0" max="99" value="0"><br><br>
          <input type="submit" value="Save">
        </form>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/set", HTTP_GET, handleForm);
  server.begin();
}

String fetchStreamURL(const char* slug) {
  String baseUrl = "https://radia.cz/api/v1/radio/";
  String url = baseUrl + String(slug) + "/streams.json";

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  String streamURL = "";
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);
    streamURL = doc[0].as<String>();
  }
  http.end();
  return streamURL;
}

void playRadio(const char* slug) {
  String streamURL = fetchStreamURL(slug);

  if (streamURL == "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stream Error");
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading stream...");

  if (I2S.begin(I2S_PHILIPS_MODE, 44100, 16)) {
    I2S.end();
    if (I2S.begin(I2S_PHILIPS_MODE, 44100, 16)) {
      I2S.start();
    }
  }

  WiFiClient client;
  HTTPClient http;
  http.begin(client, streamURL);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Playing: ");
    lcd.setCursor(0, 1);
    lcd.print(radialist[selectedRadio].name);

    Stream *stream = http.getStreamPtr();
    while (stream->available()) {
      size_t available = stream->available();
      uint8_t buffer[available];
      stream->readBytes(buffer, available);
      I2S.write(buffer, available);
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  pinMode(ENCODER_BTN_PIN, INPUT_PULLUP);
  ESP32Encoder::useInternalWeakPullResistors(true);
  encoder.attachHalfQuad(2, 3);
  encoder.clearCount();

  loadPreferences();
  connectToWiFi();
  setupWebServer();

  playRadio(radialist[selectedRadio].slug);
}

void loop() {
  long newPosition = encoder.getCount() / 2;
  if (newPosition < 0) newPosition = 0;
  if (newPosition >= radialist_size) newPosition = radialist_size - 1;

  lcd.setCursor(0, 0);
  lcd.print(String(newPosition));
  lcd.setCursor(0, 1);
  lcd.print(radialist[newPosition].name);

  if (digitalRead(ENCODER_BTN_PIN) == LOW) {
    selectedRadio = newPosition;
    savePreferences();
    playRadio(radialist[selectedRadio].slug);
    delay(500);
  }
}
