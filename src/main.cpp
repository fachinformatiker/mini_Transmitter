#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <SI5351.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h>

// Erstellen Sie ein SI5351-Objekt
SI5351 si5351;

// Erstellen Sie ein OLED-Display-Objekt
#define SCREEN_WIDTH 128 // OLED-Displaybreite in Pixel
#define SCREEN_HEIGHT 64 // OLED-Displayhöhe in Pixel
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Erstellen Sie einen Webserver-Objekt
AsyncWebServer server(80);
DNSServer dns;

// Setzen Sie die Anfangsfrequenz
uint64_t freq = 60000000ULL;

// WiFi-Daten
String ssid = "Ihr_SSID";
String password = "Ihr_Passwort";

// Access Point Daten
String ap_ssid = "Ihr_AP_SSID";
String ap_password = "Ihr_AP_Passwort";

// Tonstatus
bool toneOn = false;

void setup() {
  // Initialisieren Sie das I2C-Kommunikationsprotokoll
  Wire.begin();

  // Initialisieren Sie das OLED-Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Initialisieren Sie den SI5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_freq(freq, SI5351_CLK0); // Setzen Sie die Frequenz auf 60 MHz

  // Initialisieren Sie LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Lesen Sie die gespeicherten Einstellungen
  loadConfig();

  // Initialisieren Sie den WiFiManager
  AsyncWiFiManager wifiManager(&server,&dns);
  if (!wifiManager.autoConnect(ssid.c_str(), password.c_str())) { // Versuchen Sie, sich mit dem angegebenen Netzwerk zu verbinden
    Serial.println("Failed to connect to WiFi, setting up AP...");
    if (!wifiManager.startConfigPortal(ap_ssid.c_str(), ap_password.c_str())) { // Erstellen Sie einen Access Point, wenn die Verbindung fehlschlägt
      Serial.println("Failed to start Config Portal");
    }
  }

  // Initialisieren Sie den Webserver
  initWebServer();
}

void loop() {
  // Zeigen Sie die Frequenz auf dem OLED-Display an
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Frequenz:");
  display.println(String(freq / 1000000ULL) + " MHz");
  display.display();
}

void loadConfig() {
  File file = LittleFS.open("/config.txt", "r");
  if (!file) {
    Serial.println("Failed to open config file");
  } else {
    ssid = file.readStringUntil('\n');
    password = file.readStringUntil('\n');
    freq = file.readStringUntil('\n').toInt() * 1000000ULL;
    file.close();
  }
}

void saveConfig() {
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
  } else {
    file.println(ssid);
    file.println(password);
    file.println(String(freq / 1000000ULL));
    file.close();
  }
}

void initWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>SI5351 Frequenzsteuerung</h1>";
    html += "<p>Aktuelle Frequenz: " + String(freq / 1000000ULL) + " MHz</p>";
    html += "<form action=\"/set\" method=\"get\"><label for=\"freq\">Neue Frequenz (in MHz):</label><input type=\"text\" id=\"freq\" name=\"freq\"><input type=\"submit\" value=\"Setzen\"></form>";
    html += "<form action=\"/toggle\" method=\"get\"><input type=\"submit\" value=\"Ton ein-/ausschalten\"></form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    freq = request->getParam("freq")->value().toInt() * 1000000ULL;
    si5351.set_freq(freq, SI5351_CLK0);
    saveConfig(); // Speichern Sie die Konfiguration, wenn die Frequenz geändert wird
    request->redirect("/");
  });
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    toneOn = !toneOn; // Schalten Sie den Ton ein oder aus
    if(toneOn) {
      si5351.output_enable(SI5351_CLK0, true);
    } else {
      si5351.output_enable(SI5351_CLK0, false);
    }
    request->redirect("/");
  });
  server.begin();
}