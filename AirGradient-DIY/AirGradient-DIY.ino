/*
 * This sketch connects an AirGradient DIY sensor to a WiFi network, and runs a
 * tiny HTTP server to serve air quality metrics to Prometheus.
 */

#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <U8g2lib.h>

// TODO; refactor when AirGradient library gets support for SGP41 (not supported 02/10/2023 @ v2.4.6)
#include <SensirionI2CSgp41.h>
#include <NOxGasIndexAlgorithm.h>
#include <VOCGasIndexAlgorithm.h>
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;
// sampling cycles needed for NOx conditioning
int cond_NOx_count = 10;

// Config ----------------------------------------------------------------------

// Optional.
const char* deviceId = "";

// flip display orientation along horisontal axis, for older PRO conversion kits
const bool flipDisplay = false;

// set to 'C' to use Celcius, Farenheit if set otherwise
const char temp_display = 'C';

// show AQI (Air Quality Index) instead of NOx
const bool useAQI = false;

// be verbose in serial printing for debugging of sensors
const bool verbose = false;

// Hardware options for AirGradient DIY sensor.
#define SET_PM
#define SET_CO2
#define SET_SHT
#define SET_SGP
#define SET_DISPLAY
#define staticip

// WiFi and IP connection info.
const char* ssid = "PleaseChangeMe";
const char* password = "PleaseChangeMe";
const int port = 9926;

// Uncomment the line below to configure a static IP address.
#ifdef staticip
IPAddress static_ip(192, 168, 0, 0);
IPAddress gateway(192, 168, 0, 0);
IPAddress subnet(255, 255, 255, 0);
#endif

#ifdef SET_DISPLAY
// The frequency of measurement updates.
const int updateFrequency = 5000;
#endif  // SET_DISPLAY

// Config End ------------------------------------------------------------------

#define ERROR_PMS 0x01
#define ERROR_SHT 0x02
#define ERROR_CO2 0x04
#define ERROR_SGP 0x04

AirGradient ag = AirGradient();

#ifdef SET_SHT
TMP_RH value_sht;
float prev_value_temp = -1;
int prev_value_rh = -1;
#endif

#ifdef SET_PM
int value_pm = -1;
#endif

#ifdef SET_CO2
int value_co2 = -1;
#endif

#ifdef SET_SHT
int value_tvoc = -1;
int value_nox = -1;
#endif

#ifdef SET_DISPLAY
long lastUpdate = 0;
#endif  // SET_DISPLAY

//SH1106Wire display(0x3C, SDA, SCL);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
ESP8266WebServer server(port);

void setup() {
  Serial.begin(115200);

#ifdef SET_DISPLAY
  // Init Display.
  u8g2.setBusClock(100000);
  u8g2.begin();
  if (flipDisplay) {
    u8g2.setFlipMode(1); 
  }
  updateOLEDString("Init", String(ESP.getChipId(), HEX), "");
#endif  // SET_DISPLAY

  // Enable enabled sensors.
#ifdef SET_PM
  ag.PMS_Init();
#endif  // SET_PM
#ifdef SET_CO2
  ag.CO2_Init();
#endif  // SET_CO2
#ifdef SET_SHT
  ag.TMP_RH_Init(0x44);
#endif  // SET_SHT
#ifdef SET_SGP
  sgp41.begin(Wire);

  uint16_t serialNumber[3];
  uint8_t serialNumberSize = 3;

  uint16_t error;
  char errorMessage[256];
  error = sgp41.getSerialNumber(serialNumber);

  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    if (verbose) {
      Serial.print("SerialNumber:");
      Serial.print("0x");
      for (size_t i = 0; i < serialNumberSize; i++) {
        uint16_t value = serialNumber[i];
        Serial.print(value < 4096 ? "0" : "");
        Serial.print(value < 256 ? "0" : "");
        Serial.print(value < 16 ? "0" : "");
        Serial.print(value, HEX);
      }
      Serial.println();
    }
  }

  uint16_t testResult;
  error = sgp41.executeSelfTest(testResult);
  if (error) {
    Serial.print("Error trying to execute executeSelfTest(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (testResult != 0xD400) {
    Serial.print("executeSelfTest failed with error: ");
    Serial.println(testResult);
  }

#endif  // SET_SGP

  // Set static IP address if configured.
#ifdef staticip
  WiFi.config(static_ip, gateway, subnet);
#endif  // staticip

  // Set WiFi mode to client (without this it may try to act as an AP).
  WiFi.mode(WIFI_STA);

  // Configure Hostname
  if ((deviceId != NULL) && (deviceId[0] == '\0')) {
    Serial.println("No Device ID is Defined, Defaulting to board defaults");
  } else {
    wifi_station_set_hostname(deviceId);
    WiFi.setHostname(deviceId);
  }

  // Setup and wait for WiFi.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef SET_DISPLAY
    updateOLEDString("Connecting to ", String(ssid), "");
#endif  // SET_DISPLAY
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
  updateOLEDString("Listening To", WiFi.localIP().toString() + ":" + String(port), "");
#endif  // SET_DISPLAY
}

void loop() {
  server.handleClient();
#ifdef SET_DISPLAY
  updateScreen();
#endif  // SET_DISPLAY
}

uint8_t update() {
  uint8_t result = 0;
#ifdef SET_PM
  {
    int value = ag.getPM2_Raw();
    if (value) {
      value_pm = value;
      if (verbose) {
        Serial.println("pm: " + String(value_pm));
      }
    } else {
      result += ERROR_PMS;
    }
  }
#endif  // SET_PM

#ifdef SET_CO2
  {
    int value = ag.getCO2_Raw();
    if (value > 0) {
      value_co2 = value;
      if (verbose) {
        Serial.println("co2: " + String(value_co2));
      }
    } else {
      result += ERROR_CO2;
    }
  }
#endif  // SET_CO2

#ifdef SET_SHT
  {
    TMP_RH value = ag.periodicFetchData();
    if (value.t != NULL && value.rh != NULL) {
      value_sht = value;
      prev_value_temp = value.t;
      prev_value_rh = value.rh;
      if (verbose) {
        Serial.println("t: " + String(value_sht.t) + "\t rh: " + String(value_sht.rh));
      }
    } else {
      result += ERROR_SHT;
    }
  }
#endif  // SET_SHT

#ifdef SET_SGP
  {
    uint16_t error;
    char errorMessage[256];
    uint16_t raw_tvoc = -1;
    uint16_t raw_nox = -1;
    uint16_t compensationT;
    uint16_t compensationRh;
    uint16_t defaultCompensationT = 0x6666;
    uint16_t defaultCompensationRh = 0x8000;

    // do compensation on available metrics, falling back to default indoor compensation values
    if (result & ERROR_SHT) {
      if (prev_value_temp == -1 || prev_value_rh == -1) {
        compensationT = defaultCompensationT;
        compensationRh = defaultCompensationRh;
      } else {
        compensationT = static_cast<uint16_t>((value_sht.t + 45) * 65535 / 175);
        compensationRh = static_cast<uint16_t>(value_sht.rh * 65535 / 100);
      }
    }

    // do conditioning for 10 cycles before reporting
    if (cond_NOx_count > 0) {
      error = sgp41.executeConditioning(compensationRh, compensationT, raw_tvoc);
      cond_NOx_count--;
      if (verbose) {
        Serial.println("Conditioning NOx, count: " + String(cond_NOx_count));
      }
    } else {
      error = sgp41.measureRawSignals(compensationRh, compensationT, raw_tvoc, raw_nox);
    }

    if (error) {
      errorToString(error, errorMessage, 256);
      Serial.println("TVOC error: " + String(errorMessage));
      result += ERROR_SGP;
    } else if (cond_NOx_count <= 0) {
      value_tvoc = voc_algorithm.process(raw_tvoc);
      value_nox = nox_algorithm.process(raw_nox);
      if (verbose) {
        Serial.print("raw_tvoc: " + String(raw_tvoc) + "\traw_nox: " + String(raw_nox));
        Serial.println("Corrected tvoc: " + String(value_tvoc) + "\t nox: " + String(value_nox));
      }
    }
  }
#endif  // SET_SGP

#ifdef SET_DISPLAY
  lastUpdate = millis();
#endif
  return result;
}

String GenerateMetrics() {
  String message = "";
  String idString = "{id=\"" + String(deviceId) + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";

  // Update sensor data
  uint8_t error = update();

#ifdef SET_PM
  if (!(error & ERROR_PMS)) {
    message += "# HELP pm02 Particulate Matter PM2.5 value\n";
    message += "# TYPE pm02 gauge\n";
    message += "pm02";
    message += idString;
    message += String(value_pm);
    message += "\n";
  }
#endif  // SET_PM

#ifdef SET_CO2
  if (!(error & ERROR_CO2)) {
    message += "# HELP rco2 CO2 value, in ppm\n";
    message += "# TYPE rco2 gauge\n";
    message += "rco2";
    message += idString;
    message += String(value_co2);
    message += "\n";
  }
#endif  // SET_CO2

#ifdef SET_SHT
  if (!(error & ERROR_SHT)) {
    message += "# HELP atmp Temperature, in degrees Celsius\n";
    message += "# TYPE atmp gauge\n";
    message += "atmp";
    message += idString;
    message += String(value_sht.t);
    message += "\n# HELP rhum Relative humidity, in percent\n";
    message += "# TYPE rhum gauge\n";
    message += "rhum";
    message += idString;
    message += String(value_sht.rh);
    message += "\n";
  }
#endif  // SET_SHT

#ifdef SET_SGP
  if (!(error & ERROR_SGP)) {
    message += "# HELP tvoc Total Volatile Organic Compounts, in parts per billion\n";
    message += "# TYPE tvoc gauge\n";
    message += "voc";
    message += idString;
    message += String(value_tvoc);
    message += "\n# HELP nox Nitric Oxide, in parts per billion\n";
    message += "# TYPE nox gauge\n";
    message += "nox";
    message += idString;
    message += String(value_nox);
    message += "\n";
  }
#endif  // SET_SGP

  return message;
}

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics());
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

void updateOLED() {
  String ln1 = "PM:" + String(value_pm) + " CO2:" + String(value_co2);

  String ln2;
  if (useAQI) {
    ln2 = "AQI:" + String(PM_TO_AQI_US(value_pm)) + " TVOC:" + String(value_tvoc);
  } else {
    ln2 = "TVOC:" + String(value_tvoc) + " NOX:" + String(value_nox);
  }

  String ln3;
  if (temp_display != 'C') {
    ln3 = "F:" + String((value_sht.t * 9 / 5) + 32) + " H:" + String(value_sht.rh) + "%";
  } else {
    ln3 = "C:" + String(value_sht.t) + " H:" + String(value_sht.rh) + "%";
  }
  updateOLEDString(ln1, ln2, ln3);
}

void updateOLEDString(String ln1, String ln2, String ln3) {
  char buf[9];
  u8g2.firstPage();  // why twice?
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

void updateScreen() {
  update();
  updateOLED();
  delay(updateFrequency);
}

// Calculate PM2.5 US AQI (from DIY_PRO_SENSIRION_NOX AirGradient example)
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};

#endif  // SET_DISPLAY
