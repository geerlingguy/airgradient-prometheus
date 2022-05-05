/**
 * This sketch connects an AirGradient DIY sensor to a WiFi network, and runs a
 * tiny HTTP server to serve air quality metrics to Prometheus.
 */

#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"

// Config ----------------------------------------------------------------------

// Optional.
const char* deviceId = "";

// set to 'F' to switch display from Celcius to Fahrenheit
char temp_display = 'C';

// Hardware options for AirGradient DIY sensor.
#define SET_PM
#define SET_CO2
#define SET_SHT
#define SET_DISPLAY

// WiFi and IP connection info.
const char* ssid = "PleaseChangeMe";
const char* password = "PleaseChangeMe";
const int port = 9926;

// Uncomment the line below to configure a static IP address.
// #define staticip
#ifdef staticip
IPAddress static_ip(192, 168, 0, 0);
IPAddress gateway(192, 168, 0, 0);
IPAddress subnet(255, 255, 255, 0);
#endif

#ifdef SET_DISPLAY
// The frequency of measurement updates.
const int updateFrequency = 5000;
const int displayTime = 5000;
#endif // SET_DISPLAY

// Config End ------------------------------------------------------------------

AirGradient ag = AirGradient();

TMP_RH value_sht;
int value_pm;
int value_co2;

#ifdef SET_DISPLAY
long lastUpdate = 0;
#endif // SET_DISPLAY

SSD1306Wire display(0x3c, SDA, SCL);
ESP8266WebServer server(port);

void setup() {
  Serial.begin(9600);

#ifdef SET_DISPLAY
  // Init Display.
  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(), HEX), true);
#endif // SET_DISPLAY

  // Enable enabled sensors.
#ifdef SET_PM
  ag.PMS_Init();
#endif // SET_PM
#ifdef SET_CO2
  ag.CO2_Init();
#endif // SET_CO2
#ifdef SET_SHT
  ag.TMP_RH_Init(0x44);
#endif // SET_SHT

  // Set static IP address if configured.
#ifdef staticip
  WiFi.config(static_ip, gateway, subnet);
#endif // staticip

  // Set WiFi mode to client (without this it may try to act as an AP).
  WiFi.mode(WIFI_STA);

  // Configure Hostname
  if ((deviceId != NULL) && (deviceId[0] == '\0')) {
    Serial.printf("No Device ID is Defined, Defaulting to board defaults");
  }
  else {
    wifi_station_set_hostname(deviceId);
    WiFi.setHostname(deviceId);
  }

  // Setup and wait for WiFi.
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef SET_DISPLAY
    showTextRectangle("Trying to", "connect...", true);
#endif // SET_DISPLAY
    Serial.print(".");
  }

  Serial.print("\nConnected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + ":" + String(port));
#ifdef SET_DISPLAY
  showTextRectangle("Listening To", WiFi.localIP().toString() + ":" + String(port), true);
#endif // SET_DISPLAY
}

void loop() {
  server.handleClient();
#ifdef SET_DISPLAY
  updateScreen(millis());
#endif // SET_DISPLAY
}

void update() {
  int value = 0;
#ifdef SET_PM
  value = ag.getPM2_Raw();
  if(value)
    value_pm = value;
#endif // SET_PM

#ifdef SET_CO2
  value = ag.getCO2_Raw();
  if(value)
    value_co2 = value;
#endif // SET_CO2

#ifdef SET_SHT
  value_sht = ag.periodicFetchData();
#endif // SET_SHT
}

String GenerateMetrics() {
  String message = "";
  String idString = "{id=\"" + String(deviceId) + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";

  update();
#ifdef SET_DISPLAY
  lastUpdate = millis();
#endif

#ifdef SET_PM
  message += "# HELP pm02 Particulate Matter PM2.5 value\n";
  message += "# TYPE pm02 gauge\n";
  message += "pm02";
  message += idString;
  message += String(value_pm);
  message += "\n";
#endif // SET_PM

#ifdef SET_CO2
  message += "# HELP rco2 CO2 value, in ppm\n";
  message += "# TYPE rco2 gauge\n";
  message += "rco2";
  message += idString;
  message += String(value_co2);
  message += "\n";
#endif // SET_CO2

#ifdef SET_SHT
  message += "# HELP atmp Temperature, in degrees Celsius\n";
  message += "# TYPE atmp gauge\n";
  message += "atmp";
  message += idString;
  message += String(value_sht.t);
  message += "\n";

  message += "# HELP rhum Relative humidity, in percent\n";
  message += "# TYPE rhum gauge\n";
  message += "rhum";
  message += idString;
  message += String(value_sht.rh);
  message += "\n";
#endif // SET_SHT

  return message;
}

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics() );
}

void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}

// DISPLAY
#ifdef SET_DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

void updateScreen(long now) {
  static long lastDisplayUpdate = millis();
  static uint8_t state = 0;

  if ((now - lastUpdate) > updateFrequency) {
    // Take a measurement at a fixed interval.
    update();
    lastUpdate = millis();
  }
  
  switch (state) {
    case 0:
#ifdef SET_PM
      showTextRectangle("PM2", String(value_pm), false);
      break;
#else
      state = 1;
#endif // SET_PM
    case 1:
#ifdef SET_CO2
      showTextRectangle("CO2", String(value_co2), false);
      break;
#else
      state = 2;
#endif // SET_CO2
    case 2:
#ifdef SET_SHT
      if (temp_display == 'F' || temp_display == 'f') {
        showTextRectangle("TMP", String((value_sht.t * 9 / 5) + 32, 1) + "F", false);
      } else {
        showTextRectangle("TMP", String(value_sht.t, 1) + "C", false);
      }
      break;
    case 3:
      showTextRectangle("HUM", String(value_sht.rh) + "%", false);
#else
      state = 1;
#endif // SET_SHT
      break;
  }
  if ((now - lastDisplayUpdate) > displayTime) {
    state = (state + 1) % 4;
    lastDisplayUpdate = millis();
  }
}
#endif // SET_DISPLAY
