/************************************************************
 * EGG ROTTEN DETECTION SYSTEM – ESP8266
 * Sensors: BME680 + MQ135 + HX711
 * Output: Dynamic Localhost Web Dashboard
 ************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "HX711.h"

/* ================= WIFI ================= */
const char* ssid = "Preethi";
const char* password = "12345678";

/* ================= PINS ================= */
#define MQ135_PIN A0
#define HX711_DOUT D6
#define HX711_SCK  D5

/* ================= OBJECTS ================= */
ESP8266WebServer server(80);
Adafruit_BME680 bme;
HX711 scale;

/* ================= LOAD CELL ================= */
float calibration_factor = -191;

/* ================= SENSOR DATA ================= */
float temperature, humidity, gasResistance, weight;
int mq135;
String eggStatus = "CHECKING";

/* ================= THRESHOLDS ================= */
#define MQ135_LIMIT 420
#define GAS_LIMIT   150000
#define WEIGHT_DROP 5   // grams

/* ===== FUNCTION PROTOTYPES ===== */
void handleRoot();
void handleData();
void readSensors();


/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(D2, D1);

  /* WiFi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  /* BME680 */
  if (!bme.begin()) {
    Serial.println("BME680 NOT FOUND!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setGasHeater(320, 150);

  /* HX711 */
  scale.begin(HX711_DOUT, HX711_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();

  /* SERVER ROUTES */
  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();
  Serial.println("Web Server Started");
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();
  readSensors();
}

/* ================= SENSOR READ ================= */
void readSensors() {
  if (!bme.performReading()) return;

  temperature = bme.temperature;
  humidity = bme.humidity;
  gasResistance = bme.gas_resistance;
  mq135 = analogRead(MQ135_PIN);
  weight = scale.get_units(5);

  if (mq135 > MQ135_LIMIT || gasResistance < GAS_LIMIT || weight < 0) {
    eggStatus = "SPOILED";
  } else {
    eggStatus = "FRESH";
  }

  Serial.println("----- SENSOR DATA -----");
  Serial.println("Temp: " + String(temperature));
  Serial.println("Humidity: " + String(humidity));
  Serial.println("Gas: " + String(gasResistance));
  Serial.println("MQ135: " + String(mq135));
  Serial.println("Weight: " + String(weight));
  Serial.println("STATUS: " + eggStatus);
}


/* ================= HTML PAGE ================= */
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Egg Rotten Detection</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
* {
  box-sizing: border-box;
  font-family: 'Segoe UI', Tahoma, sans-serif;
}

body {
  margin: 0;
  min-height: 100vh;
  background: linear-gradient(135deg, #020617, #020024, #020617);
  color: #e5e7eb;
  text-align: center;
}

h1 {
  margin-top: 25px;
  font-size: 2.3rem;
  color: #22c55e;
}

.subtitle {
  color: #94a3b8;
  margin-bottom: 30px;
}

.dashboard {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 20px;
  padding: 0 25px;
  max-width: 1100px;
  margin: auto;
}

.card {
  background: rgba(255,255,255,0.08);
  backdrop-filter: blur(10px);
  border-radius: 18px;
  padding: 22px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.4);
  transition: transform 0.2s ease;
}

.card:hover {
  transform: translateY(-6px);
}

.icon {
  font-size: 32px;
  margin-bottom: 10px;
}

.label {
  color: #cbd5f5;
  font-size: 0.95rem;
}

.value {
  font-size: 1.8rem;
  margin-top: 8px;
  font-weight: bold;
}

.status-box {
  margin: 35px auto;
  padding: 20px 30px;
  border-radius: 16px;
  font-size: 1.8rem;
  width: fit-content;
  background: rgba(255,255,255,0.1);
}

.good {
  color: #22c55e;
}

.bad {
  color: #ef4444;
}
</style>
</head>

<body>

<h1>Egg Rotten Detection System</h1>
<div class="subtitle">Real-time Sensor Monitoring (ESP8266 Localhost)</div>

<div class="dashboard">
  <div class="card">
    <div class="icon">🌡️</div>
    <div class="label">Temperature</div>
    <div class="value"><span id="temp">--</span> °C</div>
  </div>

  <div class="card">
    <div class="icon">💧</div>
    <div class="label">Humidity</div>
    <div class="value"><span id="hum">--</span> %</div>
  </div>

  <div class="card">
    <div class="icon">🧪</div>
    <div class="label">Gas Resistance</div>
    <div class="value"><span id="gas">--</span></div>
  </div>

  <div class="card">
    <div class="icon">💨</div>
    <div class="label">MQ135 Value</div>
    <div class="value"><span id="mq">--</span></div>
  </div>

  <div class="card">
    <div class="icon">⚖️</div>
    <div class="label">Egg Weight</div>
    <div class="value"><span id="wt">--</span> g</div>
  </div>
</div>

<div class="status-box">
  Status: <span id="status">---</span>
</div>

<script>
setInterval(() => {
  fetch("/data")
    .then(res => res.json())
    .then(d => {
      temp.innerHTML = d.temp;
      hum.innerHTML = d.hum;
      gas.innerHTML = d.gas;
      mq.innerHTML = d.mq;
      wt.innerHTML = d.wt;

      status.innerHTML = d.status;
      status.className = d.status === "SPOILED" ? "bad" : "good";

    });
}, 1000);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}

/* ================= JSON DATA ================= */
void handleData() {
  String json = "{";
  json += "\"temp\":" + String(temperature,1) + ",";
  json += "\"hum\":" + String(humidity,1) + ",";
  json += "\"gas\":" + String(gasResistance) + ",";
  json += "\"mq\":" + String(mq135) + ",";
  json += "\"wt\":" + String(weight,1) + ",";
  json += "\"status\":\"" + eggStatus + "\"";
  json += "}";

  server.send(200, "application/json", json);
}
