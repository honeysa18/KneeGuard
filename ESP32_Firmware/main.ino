#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

// This new header file contains your entire React web application
#include "webapp.h"

Preferences prefs;
Adafruit_MPU6050 mpu;

const int piezoPin = 35;

float angleX;
unsigned long prevTime;

const char* ssid = "Honeysa";
const char* password = "harini05";

WebServer server(80);
float correctedAngle = 0;
float vibrationMV = 0;  // CHANGED: Now stores voltage in mV

// --- Web Handlers ---

/**
 * NEW: Handles the root "/" request.
 * Serves the complete HTML/CSS/JS web application.
 */
void handleRoot() {
  server.send(200, "text/html", HTML_CONTENT);
}

/**
 * MODIFIED: Handles the "/data" request.
 * Serves the JSON sensor data with voltage in mV.
 */
void handleData() {
  // CHANGED: Send vibration_mv instead of piezo
  String message = "{\"angle\": " + String(correctedAngle) + 
                   ", \"vibration_mv\": " + String(vibrationMV, 1) + "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", message);
}

// Calibration offset
float CALIB_OFFSET = 0.0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);   // ESP32: SDA=21, SCL=22

  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  prevTime = millis();

  prefs.begin("calib", false);
  CALIB_OFFSET = prefs.getFloat("offset", 0.0);
  Serial.print("Loaded calibration offset: ");
  Serial.println(CALIB_OFFSET);

  // Wi-Fi connect with retry logic
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot); 
  server.on("/data", handleData);
  
  server.begin();
}

void loop() {
  server.handleClient();

  // --- MPU6050 readings ---
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long currTime = millis();
  float dt = (currTime - prevTime) / 1000.0f;
  prevTime = currTime;

  float accAngleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
  float gyroRateX = g.gyro.x * 180.0f / PI;

  angleX = 0.98f * (angleX + gyroRateX * dt) + 0.02f * accAngleX;

  float rawAngle = (180.0f - angleX) + CALIB_OFFSET;
  if (rawAngle < 0) rawAngle = 0;
  correctedAngle = rawAngle;

  // --- MODIFIED: Piezo RMS Vibration Calculation with Voltage Conversion ---
  const int piezoSamples = 200;
  const int piezoDelay = 1;
  long sum = 0;
  int rawArr[piezoSamples];

  // Step 1: collect samples
  for (int i = 0; i < piezoSamples; i++) {
    rawArr[i] = analogRead(piezoPin);
    sum += rawArr[i];
    delay(piezoDelay);
  }

  // Step 2: compute mean
  float mean = (float)sum / piezoSamples;

  // Step 3: compute RMS around mean
  long sumSq = 0;
  for (int i = 0; i < piezoSamples; i++) {
    float centered = rawArr[i] - mean;
    sumSq += (long)(centered * centered);
  }

  int rmsADC = (int)sqrt((float)sumSq / piezoSamples);

  // NEW: Convert RMS ADC to voltage (mV)
  vibrationMV = (rmsADC / 4095.0) * 3300.0;  // Convert to millivolts

  // --- Noise filter: make base = 0 ---
  if (vibrationMV < 10) vibrationMV = 0;  // 10 mV noise threshold

  // --- MODIFIED: Serial output with voltage ---
  Serial.print(correctedAngle, 1);
  Serial.print(",");
  Serial.println(vibrationMV, 1);  // Print voltage with 1 decimal place

  // --- Web input handling for calibration offset ---
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("set ")) {
      float newOffset = input.substring(4).toFloat();
      CALIB_OFFSET = newOffset;
      prefs.putFloat("offset", CALIB_OFFSET);
      Serial.print("New offset saved: ");
      Serial.println(CALIB_OFFSET);
    }
  }

  delay(50);
}
